#include "Dashboard.h"

#ifdef MINIPLAYER_ENABLE_TUI

#include "RingBufferSink.h"

#include "../players/PlayListManager.h"
#include "../players/Schedule.h"
#include "../players/SequencePlayer.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/loop.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <thread>

namespace
{
	std::string FormatClockTime(std::chrono::system_clock::time_point tp)
	{
		std::time_t const t = std::chrono::system_clock::to_time_t(tp);
		std::tm local{};
#ifdef _WIN32
		localtime_s(&local, &t);
#else
		localtime_r(&t, &local);
#endif
		char buf[16]{};
		snprintf(buf, sizeof(buf), "%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);
		return buf;
	}

	std::string FormatDuration(uint32_t ms)
	{
		uint32_t const totalSeconds = ms / 1000;
		char buf[16]{};
		snprintf(buf, sizeof(buf), "%02u:%02u", totalSeconds / 60, totalSeconds % 60);
		return buf;
	}

	// "HH:MM:SS.mmm" -> "HH:MM:SS"
	std::string ShortTime(std::chrono::milliseconds time)
	{
		return ScheduleJson::FormatTime(time).substr(0, 8);
	}
}

Dashboard::Dashboard(SequencePlayer const& player, PlayListManager const& playlists,
	RingBufferSink const& logSink, std::string showFolder)
	: m_player(player), m_playlists(playlists), m_logSink(logSink), m_showFolder(std::move(showFolder))
{
}

void Dashboard::Run(std::function<bool()> const& shouldStop, std::function<void()> const& onTick)
{
	using namespace ftxui;

	auto screen = ScreenInteractive::Fullscreen();

	auto renderer = Renderer([&]
		{
			Elements rows;
			rows.push_back(text("mini_light_player - " + m_showFolder) | bold);
			rows.push_back(separator());

			if (m_player.IsPlaying())
			{
				uint32_t const totalMS = m_player.GetDurationMS();
				uint32_t const elapsedMS = m_player.GetCurrentFrame() *
					static_cast<uint32_t>(m_player.GetStepTimeMS());
				float const fraction = totalMS > 0
					? static_cast<float>(elapsedMS) / static_cast<float>(totalMS)
					: 0.0f;
				auto const endsAt = m_player.GetStartedAt() + std::chrono::milliseconds(totalMS);

				rows.push_back(text("Now playing: " + m_player.GetSequenceName())
					| bold | color(Color::Green));
				if (m_player.IsMusic() && !m_player.GetMediaName().empty())
				{
					rows.push_back(text("  Media: " + m_player.GetMediaName()));
				}
				rows.push_back(hbox({
					text(FormatDuration(elapsedMS) + " "),
					gauge(fraction) | flex,
					text(" " + FormatDuration(totalMS)),
				}));
				rows.push_back(text("  Started " + FormatClockTime(m_player.GetStartedAt()) +
					"   Ends " + FormatClockTime(endsAt)));
			}
			else
			{
				rows.push_back(text("Not playing") | dim);
			}

			rows.push_back(separator());

			std::string const currentPlaylist = m_playlists.GetCurrentPlaylistName();
			rows.push_back(text("Playlist: " +
				(currentPlaylist.empty() ? std::string("(none)") : currentPlaylist)));

			if (auto const active = m_playlists.GetActiveSchedule(); active)
			{
				Schedule const& s = active->get();
				rows.push_back(text("Active schedule: " + s.PlayListName + "  " +
					ShortTime(s.StartTime) + " - " + ShortTime(s.EndTime)) | color(Color::Cyan));
			}
			else
			{
				rows.push_back(text("Active schedule: (none)") | dim);
			}

			rows.push_back(separator());
			rows.push_back(text("Schedules:") | bold);
			for (auto const& s : m_playlists.GetSchedules())
			{
				std::string line = "  " + s.PlayListName + "  " +
					ShortTime(s.StartTime) + " - " + ShortTime(s.EndTime) +
					(s.Enabled ? "" : "  (disabled)");
				rows.push_back(text(line));
			}

			rows.push_back(separator());
			rows.push_back(text("Outputs: " + std::to_string(m_player.GetOutputCount()) +
				"  Channels: " + std::to_string(m_player.GetChannelCount())));

			rows.push_back(separator());
			rows.push_back(text("Log:") | bold);
			for (auto const& line : m_logSink.Lines())
			{
				rows.push_back(text(line));
			}

			rows.push_back(filler());
			rows.push_back(text("Press q to quit") | dim);

			return vbox(rows) | border;
		});

	auto withQuit = CatchEvent(renderer, [&](Event event)
		{
			if (event == Event::Character('q') || event == Event::Character('Q'))
			{
				screen.Exit();
				return true;
			}
			return false;
		});

	Loop loop(&screen, withQuit);

	// RunOnce() every 50ms keeps the UI responsive; onTick (ReloadIfChanged/
	// CheckSchedule) only needs the original ~500ms cadence.
	int ticks{ 0 };
	while (!loop.HasQuitted() && !shouldStop())
	{
		if (ticks % 10 == 0)
		{
			onTick();
		}
		++ticks;

		loop.RunOnce();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

#endif
