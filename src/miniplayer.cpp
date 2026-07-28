#include "miniplayer.h"

#include "spdlog/spdlog.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <chrono>
#include <filesystem>
#include <thread>
#include <utility>

MiniPlayer::MiniPlayer(std::string showfolder): m_showfolder(std::move(showfolder))
{
	auto const log_name{ "log.txt" };

	try
	{
		auto file{ log_name };
		std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
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

	m_logger->info("Show folder: {}", m_showfolder);

	// The logger must exist before these, they look it up by name.
	m_player = std::make_unique<SequencePlayer>();
	m_playlists = std::make_unique<PlayListManager>();
	m_player->LoadConfigs(m_showfolder);
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

void MiniPlayer::Run()
{
	while (m_player->IsPlaying())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}
