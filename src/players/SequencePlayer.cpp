#include "SequencePlayer.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <limits>
#include <vector>

SequencePlayer::SequencePlayer():
	m_logger(spdlog::get("miniplayer"))
{
	memset(m_seqData, 0, sizeof(m_seqData));

	m_syncManager = std::make_unique<SyncManager>();
	m_audio = std::make_unique<AudioPlayer>();
}

SequencePlayer::~SequencePlayer()
{
	StopSequence();
}

void SequencePlayer::LoadConfigs(std::string const& configPath)
{
	m_showFolder = configPath;
	LoadOutputs(configPath + "/" + "xlights_networks.xml");
}

void SequencePlayer::LoadSequence(std::string const& sequencePath, std::string const& mediaPath)
{
	StopSequence();

	if (!LoadSeqFile(sequencePath))
	{
		m_logger->error("Unable to load sequence file: {}", sequencePath);
		return;
	}

	m_seqFileName = std::filesystem::path( sequencePath ).filename().string();

	// An explicit media path wins over the one recorded inside the fseq.
	if(!mediaPath.empty())
	{
		m_mediaFile = mediaPath;
	}

	// The fseq only stores a bare file name, so look it up under the show folder.
	if (!m_mediaFile.empty() && !std::filesystem::exists(m_mediaFile) && !m_showFolder.empty())
	{
		std::filesystem::path const wanted = std::filesystem::path(m_mediaFile).filename();
		std::error_code ec;
		for (auto const& entry : std::filesystem::recursive_directory_iterator(
				m_showFolder, std::filesystem::directory_options::skip_permission_denied, ec))
		{
			if (entry.is_regular_file(ec) && entry.path().filename() == wanted)
			{
				m_mediaFile = entry.path().string();
				break;
			}
		}
	}

	m_mediaName = m_mediaFile.empty()
		? std::string()
		: std::filesystem::path( m_mediaFile ).filename().string();

	if(!m_mediaFile.empty() && std::filesystem::exists(m_mediaFile))
	{
		m_seqType = SeqType::Music;
	}
	else
	{
		if (!m_mediaFile.empty())
		{
			m_logger->warn("Media file not found, playing as an animation: {}", m_mediaFile);
		}
		m_seqType = SeqType::Animation;
	}
	PlaySequence();
}

void SequencePlayer::PlaySequence()
{
	if (m_seqStepTime <= 0)
	{
		m_logger->error("Sequence {} has an invalid frame time of {}ms", m_seqFileName, m_seqStepTime);
		return;
	}

	if (!m_outputManager->OpenOutputs())
	{
		m_logger->error("No outputs could be opened, not playing {}", m_seqFileName);
		return;
	}
	m_syncManager->OpenOutputs();

	if (SeqType::Music == m_seqType)
	{
		if (m_audio->Load(m_mediaFile) && m_audio->Play())
		{
			m_logger->info("Playing media {}", m_mediaName);
		}
		else
		{
			// Better to run the lights off the wall clock than not at all.
			m_logger->warn("Falling back to animation timing for {}", m_seqFileName);
			m_seqType = SeqType::Animation;
		}
	}

	m_logger->info("Playing {} - {} frames at {}ms ({}s)",
		m_seqFileName, m_numberofFrame, m_seqStepTime, m_seqMSDuration / 1000);

	m_playing = true;
	m_playbackThread = std::thread(&SequencePlayer::PlaybackLoop, this);
}

void SequencePlayer::PlaybackLoop()
{
	using clock = std::chrono::steady_clock;

	auto const start = clock::now();
	auto const step = std::chrono::milliseconds(m_seqStepTime);

	// Sentinel that no frame has been sent, so frame 0 is never skipped.
	uint32_t lastSent = std::numeric_limits<uint32_t>::max();
	uint64_t lastReportedSecond = std::numeric_limits<uint64_t>::max();

	while (m_playing)
	{
		uint64_t elapsedMS{ 0 };

		if (SeqType::Music == m_seqType)
		{
			// The audio cursor is the master clock, so the lights cannot drift
			// away from the music over a long sequence.
			if (!m_audio->IsPlaying())
			{
				break;
			}
			elapsedMS = m_audio->GetPositionMS();
		}
		else
		{
			elapsedMS = std::chrono::duration_cast<std::chrono::milliseconds>(
				clock::now() - start).count();
		}

		uint32_t const frame = static_cast<uint32_t>(elapsedMS / m_seqStepTime);

		if (frame >= m_numberofFrame)
		{
			break;
		}

		if (frame != lastSent)
		{
			OutputFrame(frame);
			lastSent = frame;

			// Once a second, so a long show leaves a usable trace without
			// flooding the log at 40fps.
			if (elapsedMS / 1000 != lastReportedSecond)
			{
				lastReportedSecond = elapsedMS / 1000;
				m_logger->debug("{} - frame {}/{} at {}s/{}s",
					m_seqFileName, frame, m_numberofFrame,
					elapsedMS / 1000, m_seqMSDuration / 1000);
			}
		}

		if (SeqType::Music == m_seqType)
		{
			// Poll faster than the frame rate so we land close to each boundary.
			std::this_thread::sleep_for(std::min(step / 2, std::chrono::milliseconds(10)));
		}
		else
		{
			// Absolute deadlines, so a slow frame does not push the whole show late.
			std::this_thread::sleep_until(start + step * (frame + 1));
		}
	}

	Shutdown();
}

void SequencePlayer::OutputFrame(uint32_t frame)
{
	m_lastFrameData.reset(m_seqFile->getFrame(frame));
	if (m_lastFrameData == nullptr)
	{
		return;
	}

	m_lastFrameData->readFrame((uint8_t*)m_seqData, FPPD_MAX_CHANNELS);
	m_outputManager->OutputData((uint8_t*)m_seqData);
	SendSync(frame);
}

void SequencePlayer::Blackout()
{
	memset(m_seqData, 0, sizeof(m_seqData));
	m_outputManager->OutputData((uint8_t*)m_seqData);
}

void SequencePlayer::Shutdown()
{
	// Leaving the props lit on the last frame would look like a hung show.
	Blackout();

	m_syncManager->SendStop();

	m_audio->Stop();
	m_audio->Unload();

	m_outputManager->CloseOutputs();
	m_syncManager->CloseOutputs();

	m_logger->info("Sequence ended: {}", m_seqFileName);

	m_playing = false;
}

void SequencePlayer::StopSequence()
{
	m_playing = false;
	if (m_playbackThread.joinable())
	{
		m_playbackThread.join();
	}
}

void SequencePlayer::LoadOutputs(std::string const& configPath)
{
	m_outputManager = std::make_unique<OutputManager>();
	m_outputManager->LoadOutputs(configPath);
}

void SequencePlayer::SendSync(uint32_t frameIdx)
{
	m_syncManager->SendSync(m_seqStepTime, frameIdx, m_seqFileName, m_mediaName);
}

bool SequencePlayer::LoadSeqFile(std::string const& sequencePath)
{
	// Releases any previously loaded sequence.
	m_lastFrameData.reset();
	m_seqFile.reset(FSEQFile::openFSEQFile(sequencePath));
	if (m_seqFile == nullptr)
	{
		return false;
	}
	// getFrame() would do this lazily on the first frame; doing it here keeps
	// the decompression setup out of the frame loop.
	std::vector<std::pair<uint32_t, uint32_t>> ranges;
	ranges.emplace_back(0, m_seqFile->getMaxChannel() + 1);
	m_seqFile->prepareRead(ranges, 0);

	m_seqStepTime = m_seqFile->getStepTime();

	m_mediaFile = m_seqFile->getMediaFilename();

	m_seqMSDuration = m_seqFile->getNumFrames() * m_seqFile->getStepTime();
	m_numberofFrame = m_seqFile->getNumFrames();
	return true;
}

void SequencePlayer::SetMultisync(bool enabled)
{
	m_syncManager->SetEnabled(enabled);
}
