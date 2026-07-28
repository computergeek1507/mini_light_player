#include "AudioPlayer.h"

// miniaudio is header only; this is the single translation unit that compiles it.
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#include "../miniaudio.h"

#include "spdlog/spdlog.h"

struct AudioPlayer::Impl
{
	ma_engine engine{};
	ma_sound sound{};
	bool engineReady{ false };
	bool soundReady{ false };
};

AudioPlayer::AudioPlayer() : m_impl(std::make_unique<Impl>())
{
	if (ma_engine_init(nullptr, &m_impl->engine) != MA_SUCCESS)
	{
		auto logger = spdlog::get("miniplayer");
		if (logger) logger->error("Unable to initialise the audio engine");
		return;
	}
	m_impl->engineReady = true;
}

AudioPlayer::~AudioPlayer()
{
	Unload();
	if (m_impl->engineReady)
	{
		ma_engine_uninit(&m_impl->engine);
		m_impl->engineReady = false;
	}
}

bool AudioPlayer::Load(std::string const& file)
{
	auto logger = spdlog::get("miniplayer");

	if (!m_impl->engineReady)
	{
		return false;
	}

	Unload();

	// Stream rather than decode up front: fully decoding an 11 minute mp3 costs
	// several seconds of silence before the lights start and a few hundred MB of
	// PCM. miniaudio reads ahead on its own job thread, so the frame loop is not
	// exposed to disk latency.
	ma_result const result = ma_sound_init_from_file(&m_impl->engine, file.c_str(),
		MA_SOUND_FLAG_STREAM, nullptr, nullptr, &m_impl->sound);

	if (result != MA_SUCCESS)
	{
		if (logger) logger->error("Unable to load media file ({}): {}", static_cast<int>(result), file);
		return false;
	}

	m_impl->soundReady = true;
	m_loaded = true;
	return true;
}

void AudioPlayer::Unload()
{
	if (m_impl->soundReady)
	{
		ma_sound_uninit(&m_impl->sound);
		m_impl->soundReady = false;
	}
	m_loaded = false;
}

bool AudioPlayer::Play()
{
	if (!m_loaded)
	{
		return false;
	}
	return ma_sound_start(&m_impl->sound) == MA_SUCCESS;
}

void AudioPlayer::Stop()
{
	if (m_loaded)
	{
		ma_sound_stop(&m_impl->sound);
	}
}

bool AudioPlayer::IsPlaying() const
{
	return m_loaded && ma_sound_is_playing(&m_impl->sound) == MA_TRUE;
}

uint64_t AudioPlayer::GetPositionMS() const
{
	if (!m_loaded)
	{
		return 0;
	}

	ma_uint64 cursor{ 0 };
	if (ma_sound_get_cursor_in_pcm_frames(&m_impl->sound, &cursor) != MA_SUCCESS)
	{
		return 0;
	}

	// The cursor counts frames of the engine's output, not of the source file.
	ma_uint32 const rate = ma_engine_get_sample_rate(&m_impl->engine);
	if (rate == 0)
	{
		return 0;
	}
	return (cursor * 1000ULL) / rate;
}

uint64_t AudioPlayer::GetDurationMS() const
{
	if (!m_loaded)
	{
		return 0;
	}

	float seconds{ 0.0f };
	if (ma_sound_get_length_in_seconds(&m_impl->sound, &seconds) != MA_SUCCESS)
	{
		return 0;
	}
	return static_cast<uint64_t>(seconds * 1000.0f);
}
