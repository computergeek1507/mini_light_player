#ifndef MINI_PLAYER_H
#define MINI_PLAYER_H

#include "./players/SequencePlayer.h"
#include "./players/PlayListManager.h"

#include "spdlog/spdlog.h"
#include "spdlog/common.h"

#include <memory>
#include <filesystem>

#ifdef MINIPLAYER_ENABLE_TUI
class RingBufferSink;
#endif

class MiniPlayer
{
public:
    // tuiMode is silently ignored (with a logged warning) in builds compiled
    // without MINIPLAYER_ENABLE_TUI.
    MiniPlayer(std::string showfolder, bool tuiMode = false);
    ~MiniPlayer();

    // Starts a sequence. sequence and media may be absolute paths or names
    // relative to the show folder.
    void Play(std::string const& sequence, std::string const& media = std::string());

    // Plays a single sequence and then exits, rather than following schedules.
    void PlayOnce(std::string const& sequence, std::string const& media = std::string());

    // Runs until the sequence ends, or forever following the schedule.
    void Run();

    // Acts as an FPP multisync master, driving remote players.
    void SetMultisync(bool enabled) { m_player->SetMultisync(enabled); }

private:
    // Resolves a possibly-relative name against the show folder, searching the
    // usual xLights subdirectories.
    std::string ResolvePath(std::string const& name) const;

    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
    std::unique_ptr<SequencePlayer> m_player{ nullptr };
    std::unique_ptr<PlayListManager> m_playlists{ nullptr };
    std::string m_appdir;
    std::string m_showfolder;
    bool m_oneShot{ false };
    bool m_tuiMode{ false };

#ifdef MINIPLAYER_ENABLE_TUI
    // A full-screen dashboard can't share a terminal with plain log lines, so
    // this sink stands in for the stdout sink while the dashboard is active.
    std::shared_ptr<RingBufferSink> m_logSink{ nullptr };
#endif

};

#endif // MINI_PLAYER_H
