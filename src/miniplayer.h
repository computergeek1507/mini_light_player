#ifndef MINI_PLAYER_H
#define MINI_PLAYER_H

#include "./players/SequencePlayer.h"
#include "./players/PlayListManager.h"

#include "spdlog/spdlog.h"
#include "spdlog/common.h"

#include <memory>
#include <filesystem>

class MiniPlayer
{
public:
    MiniPlayer(std::string showfolder);

private:
    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
    std::unique_ptr<SequencePlayer> m_player{ nullptr };
    std::unique_ptr<PlayListManager> m_playlists{ nullptr };
    std::string m_appdir;
    std::string m_showfolder;

}

#endif // MINI_PLAYER_H