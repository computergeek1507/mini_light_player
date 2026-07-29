#ifndef SEQUENCEPLAYER_H
#define SEQUENCEPLAYER_H

#include "../fseq/FSEQFile.h"

#include "../audio/AudioPlayer.h"
#include "../outputs/OutputManager.h"
#include "../outputs/SyncManager.h"

#include "spdlog/spdlog.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <chrono>
#include <string>
#include <thread>

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

    SequencePlayer(SequencePlayer const&) = delete;
    SequencePlayer& operator=(SequencePlayer const&) = delete;

    void LoadConfigs(std::string const& configPath);

    void LoadSequence(std::string const& sequencePath, std::string const& mediaPath = std::string());

    // Stops playback and blocks until the playback thread has finished.
    void StopSequence();

    void LoadOutputs(std::string const& configPath);
    void SendSync(uint32_t frameIdx);

    void SetMultisync(bool enabled);

    [[nodiscard]] bool IsPlaying() const { return m_playing; }

    [[nodiscard]] uint32_t GetDurationMS() const { return m_seqMSDuration; }
    [[nodiscard]] std::string const& GetSequenceName() const { return m_seqFileName; }

private:
    void PlaySequence();
    bool LoadSeqFile(std::string const& sequencePath);

    // Runs on m_playbackThread until the sequence ends or m_playing clears.
    void PlaybackLoop();

    // Reads one frame from the fseq and pushes it to every output.
    void OutputFrame(uint32_t frame);

    // Sends a frame of zeros so the props do not hold their last colour.
    void Blackout();

    // Releases outputs and audio. Called once, from the playback thread.
    void Shutdown();

    std::string m_showFolder;
    std::string m_seqFileName;
    std::string m_mediaFile;
    std::string m_mediaName;
    std::unique_ptr<FSEQFile> m_seqFile{nullptr};
    uint32_t m_seqMSDuration{0};
    uint32_t m_numberofFrame{0};

    int m_seqStepTime{0};

    SeqType m_seqType { SeqType::Animation };

    std::atomic_bool m_playing{ false };
    std::thread m_playbackThread;

    // getFrame() hands back ownership, so this must not be a raw pointer:
    // overwriting it every frame leaked one FrameData per frame.
    std::unique_ptr<FSEQFile::FrameData> m_lastFrameData{nullptr};

    std::unique_ptr<AudioPlayer> m_audio{nullptr};

    std::unique_ptr<OutputManager> m_outputManager{nullptr};

    std::unique_ptr<SyncManager> m_syncManager{nullptr};

    std::shared_ptr<spdlog::logger> m_logger{ nullptr };

    char m_seqData[FPPD_MAX_CHANNELS];
};

#endif
