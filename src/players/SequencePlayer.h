#ifndef SEQUENCEPLAYER_H
#define SEQUENCEPLAYER_H

#include "../fseq/FSEQFile.h"

#include "../outputs/OutputManager.h"
#include "../outputs/SyncManager.h"

#include "spdlog/spdlog.h"

#include <memory>
#include <chrono>

#define FPPD_MAX_CHANNELS (8192 * 1024)
#define DATA_DUMP_SIZE 28

enum class SeqType
{
    Animation,
    Music
};

enum class PlaybackStatus
{
    Playing,
    Loading,
    Stopped
};

class SequencePlayer
{

public:
    SequencePlayer();
    ~SequencePlayer();

    void LoadConfigs(std::string const& configPath);


    void LoadSequence(std::string const& sequencePath, std::string const& mediaPath = std::string());

    void StopSequence();
    void LoadOutputs(std::string const& configPath);
    void SendSync(uint64_t frameIdx);


    void TriggerOutputData();
    void TriggerTimedOutputData(uint64_t timeMS);

    void SetMultisync(bool enabled);  

    void UpdateSequence(std::string const& sequenceName, std::string const& media, int frames, int frameSizeMS);
    void AddController(bool enabled, std::string const& type, std::string const& ip, std::string const& channels);
    void UpdateStatus(std::string const& message);
    void UpdateTime(std::string const& sequenceName, int elapsedMS, int durationMS);

    void UpdatePlaybackStatus(std::string const& sequencePath, PlaybackStatus status);

private:
    void PlaySequence();
    bool LoadSeqFile(std::string const& sequencePath);
    void StartAnimationSeq();

    void StartMusicSeq();

    std::string m_seqFileName;
    std::string m_mediaFile;
    std::string m_mediaName;
    FSEQFile* m_seqFile{nullptr};
    //std::chrono::time_point<std::chrono::high_resolution_clock> m_seqMSElapsed;
    int m_seqMSDuration{0};
    //int m_seqMSElapsed{0};
    //int m_seqMSRemaining{0};
    int m_lastFrameRead{0};
    int m_numberofFrame{0};

    int m_seqStepTime{0};
    //float m_seqRefreshRate{0};
    uint64_t channelsCount{0};

    std::unique_ptr<QTimer> m_playbackTimer{nullptr};
    QThread m_playbackThread;

    SeqType m_seqType { SeqType::Animation };

    //std::atomic_int m_lastFrameRead;
    FSEQFile::FrameData* m_lastFrameData{nullptr};

    std::unique_ptr<QMediaPlayer> m_mediaPlayer{nullptr};

    std::unique_ptr<OutputManager> m_outputManager{nullptr};

    std::unique_ptr<SyncManager> m_syncManager{nullptr};

    std::shared_ptr<spdlog::logger> m_logger{ nullptr };

    char m_seqData[FPPD_MAX_CHANNELS];
};

#endif