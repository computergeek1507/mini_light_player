#include "miniplayer.h"

#include "spdlog/spdlog.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#ifdef MINIPLAYER_ENABLE_TUI
#include "./tui/Dashboard.h"
#include "./tui/RingBufferSink.h"
#endif

#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <utility>

namespace
{
	// Set from the signal handler, so only an atomic flag is touched there.
	std::atomic_bool g_stopRequested{ false };

	void HandleSignal(int) { g_stopRequested = true; }
}

MiniPlayer::MiniPlayer(std::string showfolder, bool tuiMode)
	: m_showfolder(std::move(showfolder)), m_tuiMode(tuiMode)
{
	auto const log_name{ "log.txt" };

	try
	{
		auto file{ log_name };
		std::vector<spdlog::sink_ptr> sinks;

#ifdef MINIPLAYER_ENABLE_TUI
		if (m_tuiMode)
		{
			// A full-screen interactive display can't share a terminal with
			// plain stdout log lines, so keep only the file sink plus a small
			// in-memory tail the dashboard can render instead.
			m_logSink = std::make_shared<RingBufferSink>(200);
			sinks.push_back(m_logSink);
		}
		else
#endif
		{
			sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		}

		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, 1024 * 1024, 5, false));

		m_logger = std::make_shared<spdlog::logger>("miniplayer", sinks.begin(), sinks.end());
		m_logger->flush_on(spdlog::level::debug);
		m_logger->set_level(spdlog::level::debug);
		m_logger->set_pattern("[%D %H:%M:%S] [%L] %v");
		spdlog::register_logger(m_logger);
	}
	catch (std::exception& /*ex*/)
	{

	}

	if (!std::filesystem::is_directory(m_showfolder))
	{
		throw std::runtime_error("show folder not found: " + m_showfolder);
	}

#ifndef MINIPLAYER_ENABLE_TUI
	if (m_tuiMode)
	{
		m_logger->warn("--tui was requested but this build was compiled without TUI support");
		m_tuiMode = false;
	}
#endif

	m_logger->info("Show folder: {}", m_showfolder);

	// The logger must exist before these, they look it up by name.
	m_player = std::make_unique<SequencePlayer>();
	m_playlists = std::make_unique<PlayListManager>();
	m_player->LoadConfigs(m_showfolder);

	// Replaces the Qt signal that used to connect the playlist to the player.
	m_playlists->SetPlayHandler([this](std::string const& sequence, std::string const& media)
		{
			Play(sequence, media);
		});

	m_playlists->LoadPlayLists(m_showfolder);
}

MiniPlayer::~MiniPlayer() = default;

std::string MiniPlayer::ResolvePath(std::string const& name) const
{
	if (name.empty())
	{
		return name;
	}

	std::filesystem::path const given(name);
	if (given.is_absolute() && std::filesystem::exists(given))
	{
		return given.string();
	}

	// xLights keeps sequences in per-year folders and audio under Media/Audio.
	std::filesystem::path const root(m_showfolder);
	if (std::filesystem::exists(root / given))
	{
		return (root / given).string();
	}

	std::error_code ec;
	for (auto const& entry : std::filesystem::recursive_directory_iterator(
			root, std::filesystem::directory_options::skip_permission_denied, ec))
	{
		if (entry.is_regular_file(ec) && entry.path().filename() == given.filename())
		{
			return entry.path().string();
		}
	}

	return name;
}

void MiniPlayer::Play(std::string const& sequence, std::string const& media)
{
	std::string const seqPath = ResolvePath(sequence);
	if (!std::filesystem::exists(seqPath))
	{
		m_logger->error("Sequence not found: {}", sequence);
		return;
	}

	m_player->LoadSequence(seqPath, ResolvePath(media));
}

void MiniPlayer::PlayOnce(std::string const& sequence, std::string const& media)
{
	m_oneShot = true;
	Play(sequence, media);
}

void MiniPlayer::Run()
{
	// Ctrl+C must still reach the blackout, otherwise the props stay lit on
	// whatever frame they happened to be showing.
	std::signal(SIGINT, HandleSignal);
	std::signal(SIGTERM, HandleSignal);

	if (m_oneShot)
	{
		while (m_player->IsPlaying() && !g_stopRequested)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
#ifdef MINIPLAYER_ENABLE_TUI
	else if (m_tuiMode)
	{
		Dashboard dashboard(*m_player, *m_playlists, *m_logSink, m_showfolder);
		dashboard.Run(
			[] { return g_stopRequested.load(); },
			[this]
			{
				m_playlists->ReloadIfChanged();
				if (!m_player->IsPlaying())
				{
					m_playlists->UpdateStatus(std::string(), PlaybackStatus::Stopped);
					m_playlists->CheckSchedule();
				}
			});
	}
#endif
	else
	{
		m_logger->info("Watching for playlists/schedules in {}, Ctrl+C to stop", m_showfolder);

		while (!g_stopRequested)
		{
			// Checked every tick rather than only at startup, so editing the
			// config file - or adding one that didn't exist yet - takes effect
			// without restarting the player.
			m_playlists->ReloadIfChanged();

			// The schedule only advances between sequences; a running sequence
			// is always allowed to finish.
			if (!m_player->IsPlaying())
			{
				m_playlists->UpdateStatus(std::string(), PlaybackStatus::Stopped);
				m_playlists->CheckSchedule();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	if (g_stopRequested)
	{
		m_logger->info("Stop requested");
	}

	m_player->StopSequence();
}
