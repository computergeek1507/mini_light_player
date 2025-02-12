#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include "SequencePlayer.h"

#include "Schedule.h"

#include "spdlog/spdlog.h"

#include <string>
#include <memory>
#include <optional>
#include <vector>

#include <nlohmann/json.hpp>

struct PlayList;
//struct Schedule;

class PlayListManager
{


public:

    PlayListManager();
    ~PlayListManager();
    bool LoadPlayLists(std::string const& configFolder);
    void SavePlayLists(std::string const& configFolder);

    [[nodiscard]] std::optional<std::reference_wrapper<PlayList const>> GetPlayList(int index) const;
    [[nodiscard]] std::optional<std::reference_wrapper<PlayList const>> GetPlayList(std::string const& name) const;
    [[nodiscard]] std::vector<std::string> GetPlayLists() const;
    [[nodiscard]] std::vector<Schedule> const& GetSchedules() const { return m_schedules; };

    void UpdateStatus(std::string const& sequencePath, PlaybackStatus status);

    void LoadJsonFile(const std::string& jsonFile);

    void AddPlaylistName(std::string const& playlist);
    void AddSequence(std::string const& fseqPath, std::string const& mediaPath, int index);

    void PlaySequence(int playlist_index, int sequence_index) const;
    void AddSchedule(Schedule schedule);
    void EditSchedule(int schedule_index, Schedule schedule);

    void CheckSchedule();


private:

    void ReadPlaylists(nlohmann::json const& json);
    void ReadSchedules(nlohmann::json const& json);
    //void WritePlaylists(QJsonObject& json) const;
    //void WriteSchedules(QJsonObject& json) const;

    void PlayNextSequence();
    void PlayNewPlaylist(std::string const& playlistName);

    std::vector<PlayList> m_playlists;
    std::vector<Schedule> m_schedules;

    //std::unique_ptr<QTimer> m_scheduleTimer{nullptr};
   // QThread m_scheduleThread;

    std::string m_currentPlaylist;
    //QString m_currentSequence;
    int m_nextSequenceIdx{0};

    PlaybackStatus m_status{PlaybackStatus::Stopped};

    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};
#endif
