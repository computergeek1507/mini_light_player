#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include "SequencePlayer.h"

#include "PlayListItem.h"
#include "Schedule.h"

#include "spdlog/spdlog.h"

#include <functional>
#include <string>
#include <memory>
#include <optional>
#include <vector>

struct PlayList;

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
	void SaveJsonFile(const std::string& jsonFile);
    void AddPlaylistName(std::string const& playlist);
    void AddSequence(std::string const& fseqPath, std::string const& mediaPath, int index);

    void PlaySequence(int playlist_index, int sequence_index);
    void DeleteSequence(int playlist_index, int sequence_index);
    void DeletePlayList(int playlist_index);
    void MoveSequenceUp(int playlist_index, int sequence_index);
    void MoveSequenceDown(int playlist_index, int sequence_index);

    void AddSchedule(Schedule schedule);
    void EditSchedule(int schedule_index, Schedule schedule);

    void DeleteSchedule(int schedule_index);
    void MoveScheduleUp(int schedule_index);
    void MoveScheduleDown(int schedule_index);

    // Starts whatever the schedules say should be running now. Does nothing
    // while a sequence is playing.
    void CheckSchedule();

    // Replaces the Qt PlaySequenceSend signal. Called with the sequence and
    // media paths recorded in the playlist.
    using PlayHandler = std::function<void(std::string const& sequence, std::string const& media)>;
    void SetPlayHandler(PlayHandler handler) { m_playHandler = std::move(handler); }

    [[nodiscard]] bool HasSchedules() const { return !m_schedules.empty(); }

private:

    void PlayNextSequence();
    void PlayNewPlaylist(std::string const& playlistName);

    void StartSequence(PlayListItem const& item);

    std::vector<PlayList> m_playlists;
    std::vector<Schedule> m_schedules;

    PlayHandler m_playHandler;

    std::string m_currentPlaylist;
    int m_nextSequenceIdx{0};

    PlaybackStatus m_status{PlaybackStatus::Stopped};

    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};
#endif
