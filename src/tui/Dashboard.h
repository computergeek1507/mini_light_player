#ifndef DASHBOARD_H
#define DASHBOARD_H

#ifdef MINIPLAYER_ENABLE_TUI

#include <functional>
#include <string>

class SequencePlayer;
class PlayListManager;
class RingBufferSink;

// A live FTXUI status display, used instead of MiniPlayer's plain polling
// loop when --tui is passed. Takes over the terminal until 'q' or Ctrl+C.
class Dashboard
{
public:
	Dashboard(SequencePlayer const& player, PlayListManager const& playlists,
		RingBufferSink const& logSink, std::string showFolder);

	// Runs until the user presses 'q' or shouldStop() returns true. onTick is
	// called roughly every 500ms - the same cadence MiniPlayer's plain loop
	// already used for ReloadIfChanged()/CheckSchedule() - independent of the
	// dashboard's own faster redraw rate.
	void Run(std::function<bool()> const& shouldStop, std::function<void()> const& onTick);

private:
	SequencePlayer const& m_player;
	PlayListManager const& m_playlists;
	RingBufferSink const& m_logSink;
	std::string m_showFolder;
};

#endif
#endif
