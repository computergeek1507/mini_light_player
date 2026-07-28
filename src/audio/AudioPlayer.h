#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <cstdint>
#include <memory>
#include <string>

// Thin wrapper over miniaudio. The playback cursor is the master clock for
// musical sequences, so the lights follow the audio rather than a free-running
// timer that would slowly drift out of sync.
class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    AudioPlayer(AudioPlayer const&) = delete;
    AudioPlayer& operator=(AudioPlayer const&) = delete;

    // Decodes the file up front so seeking and cursor reads are cheap.
    bool Load(std::string const& file);
    void Unload();

    bool Play();
    void Stop();

    [[nodiscard]] bool IsPlaying() const;

    // Current playback position. Only meaningful while playing.
    [[nodiscard]] uint64_t GetPositionMS() const;

    [[nodiscard]] uint64_t GetDurationMS() const;

    [[nodiscard]] bool IsLoaded() const { return m_loaded; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_loaded{ false };
};

#endif
