
#include "SyncManager.h"

#include "SyncPacket.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <vector>

#define FPP_MEDIA_SYNC_INTERVAL_MS 500
#define FPP_SEQ_SYNC_INTERVAL_FRAMES 16
#define FPP_SEQ_SYNC_INTERVAL_INITIAL_FRAMES 4
#define FPP_SEQ_SYNC_INITIAL_NUMBER_OF_FRAMES 32

namespace
{
	bool IsSequenceFile(std::string const& path)
	{
		std::string ext = std::filesystem::path(path).extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return ext == ".fseq";
	}
}

SyncManager::SyncManager() :
	m_logger(spdlog::get("miniplayer"))
{
}

SyncManager::~SyncManager()
{
	CloseOutputs();
}

bool SyncManager::OpenOutputs()
{
	if (!m_enabled)
	{
		return false;
	}

	// FPP listens on a multicast group; sending needs no group membership.
	if (!m_sender.Open(MULTISYNC_MULTICAST_ADDRESS, FPP_CTRL_PORT))
	{
		m_logger->error("Multisync disabled: cannot open {}:{}",
			MULTISYNC_MULTICAST_ADDRESS, FPP_CTRL_PORT);
		return false;
	}

	m_logger->info("Multisync master on {}:{}", MULTISYNC_MULTICAST_ADDRESS, FPP_CTRL_PORT);
	return true;
}

void SyncManager::CloseOutputs()
{
	m_sender.Close();
	m_lastFseq.clear();
	m_lastMedia.clear();
	m_lastFrame = 0;
	m_lastMediaMsec = 0;
}

void SyncManager::SendSync(uint32_t frameSizeMS, uint32_t frame, std::string const& fseq, std::string const& media)
{
	if (!m_enabled || !m_sender.IsOpen())
	{
		return;
	}

	// An empty name means "stop whatever the remotes are currently running".
	if (fseq.empty())
	{
		if (!m_lastFseq.empty())
		{
			SendFPPSync(m_lastFseq, 0xFFFFFFFF, 0);
		}
		if (!m_lastMedia.empty())
		{
			SendFPPSync(m_lastMedia, 0xFFFFFFFF, 0);
		}

		m_lastFseq.clear();
		m_lastMedia.clear();
		m_lastFrame = 0;
		m_lastMediaMsec = 0;
		return;
	}

	bool dosendFSEQ{ false };
	bool dosendMedia{ false };

	if (frame == 0 || frame == 0xFFFFFFFF)
	{
		dosendFSEQ = true;
		dosendMedia = true;
	}

	if (IsSequenceFile(fseq))
	{
		if (m_lastFseq != fseq)
		{
			if (!m_lastFseq.empty())
			{
				SendFPPSync(m_lastFseq, 0xFFFFFFFF, frame);
			}

			m_lastFseq = fseq;

			// Tell the remotes to open the new file before syncing into it.
			if (frame != 0)
			{
				SendFPPSync(fseq, 0, frame);
			}
		}

		if (!dosendFSEQ)
		{
			// Sync more often over the first frames, while the remotes are
			// still settling onto the master's position.
			uint32_t const interval = (frame <= FPP_SEQ_SYNC_INITIAL_NUMBER_OF_FRAMES)
				? FPP_SEQ_SYNC_INTERVAL_INITIAL_FRAMES
				: FPP_SEQ_SYNC_INTERVAL_FRAMES;

			if (frame - m_lastFrame >= interval)
			{
				dosendFSEQ = true;
			}
		}
	}

	if (!media.empty())
	{
		if (m_lastMedia != media)
		{
			if (!m_lastMedia.empty())
			{
				SendFPPSync(m_lastMedia, 0xFFFFFFFF, frame);
			}

			m_lastMedia = media;

			// Only pre-open when joining mid file. At frame 0 the START below
			// covers it, and sending both duplicates the packet.
			if (frame != 0)
			{
				SendFPPSync(media, 0, frame);
			}
		}

		if (!dosendMedia)
		{
			uint32_t const stepMS = frame * frameSizeMS;
			if (stepMS - m_lastMediaMsec >= FPP_MEDIA_SYNC_INTERVAL_MS)
			{
				dosendMedia = true;
			}
		}
	}

	uint32_t const stepMS = frame * frameSizeMS;
	if (dosendFSEQ)
	{
		SendFPPSync(fseq, stepMS, frame);
		m_lastFrame = frame;
	}
	if (dosendMedia && !media.empty())
	{
		SendFPPSync(media, stepMS, frame);
		m_lastMediaMsec = stepMS;
	}
}

void SyncManager::SendStop()
{
	if (!m_enabled || !m_sender.IsOpen())
	{
		return;
	}
	SendSync(0, 0xFFFFFFFF, std::string(), std::string());
}

void SyncManager::SendFPPSync(const std::string& item, uint32_t stepMS, uint32_t frames)
{
	// FPP matches on the bare file name, not the master's path to it.
	std::string const name = std::filesystem::path(item).filename().string();

	size_t const bufsize = sizeof(ControlPkt) + sizeof(SyncPkt) + name.size();
	std::vector<uint8_t> buffer(bufsize, 0);

	ControlPkt* cp = reinterpret_cast<ControlPkt*>(&buffer[0]);
	memcpy(cp->fppd, "FPPD", 4);
	cp->pktType = CTRL_PKT_SYNC;
	cp->extraDataLen = static_cast<uint16_t>(bufsize - sizeof(ControlPkt));

	SyncPkt* sp = reinterpret_cast<SyncPkt*>(&buffer[0] + sizeof(ControlPkt));

	if (stepMS == 0)
	{
		sp->pktType = SYNC_PKT_START;
	}
	else if (stepMS == 0xFFFFFFFF)
	{
		sp->pktType = SYNC_PKT_STOP;
	}
	else
	{
		sp->pktType = SYNC_PKT_SYNC;
	}

	if (IsSequenceFile(name))
	{
		sp->fileType = SYNC_FILE_SEQ;
		sp->frameNumber = frames;
	}
	else
	{
		sp->fileType = SYNC_FILE_MEDIA;
		sp->frameNumber = 0;
	}

	if (sp->pktType == SYNC_PKT_SYNC)
	{
		sp->secondsElapsed = stepMS / 1000.0f;
	}
	else
	{
		sp->frameNumber = 0;
		sp->secondsElapsed = 0;
	}

	// buffer was zero filled, so the name is already NUL terminated.
	memcpy(&sp->filename[0], name.c_str(), name.size());

	m_sender.Send(buffer.data(), bufsize);
}
