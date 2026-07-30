
#include "SyncManager.h"

#include "SyncPacket.h"

#include <cstring>
#include <filesystem>
#include <vector>

// Sync throttle intervals below and the Open-then-Start call pattern were
// confirmed against FalconChristmas/fpp, not guessed:
//  - src/MultiSync.cpp: SendSeqSyncPacket sends every 4 frames for the first
//    32 frames of a sequence, then every 10 frames after that. Real FPP does
//    not throttle a media Sync packet by frame count at all - it compares
//    (int)(seconds * 2.0f) to the last value sent, i.e. a half-second bucket.
//  - src/Sequence.cpp: OpenSequenceFile() calls SendSeqOpenPacket() and
//    StartSequence() separately calls SendSeqSyncStartPacket() - two distinct
//    packets, not one. Both call sites pass a bare filename (m_seqFilename),
//    confirming FPP itself only ever sends a basename, never the master's
//    path to the file.
//  - docs/ControlProtocol.txt / src/MultiSync.h: the ControlPkt/SyncPkt wire
//    layout below already matched this project's structs exactly.
#define FPP_SEQ_SYNC_INTERVAL_FRAMES 10
#define FPP_SEQ_SYNC_INTERVAL_INITIAL_FRAMES 4
#define FPP_SEQ_SYNC_INITIAL_NUMBER_OF_FRAMES 32

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
	m_lastSyncedFrame = 0;
	m_lastMediaHalfSecond = 0;
}

void SyncManager::SendSync(uint32_t frameSizeMS, uint32_t frame, std::string const& fseq, std::string const& media)
{
	if (!m_enabled || !m_sender.IsOpen() || fseq.empty())
	{
		return;
	}

	if (m_lastFseq != fseq)
	{
		if (!m_lastFseq.empty())
		{
			SendSeqSyncStopPacket(m_lastFseq);
		}

		m_lastFseq = fseq;
		m_lastSyncedFrame = 0;

		// FPP sends these as two distinct packets when a file begins - Open
		// first (remotes can start preparing the file), then Start.
		SendSeqOpenPacket(fseq);
		SendSeqSyncStartPacket(fseq);
	}

	if (!media.empty() && m_lastMedia != media)
	{
		if (!m_lastMedia.empty())
		{
			SendMediaSyncStopPacket(m_lastMedia);
		}

		m_lastMedia = media;
		m_lastMediaHalfSecond = 0;

		SendMediaOpenPacket(media);
		SendMediaSyncStartPacket(media);
	}

	if (frame == 0)
	{
		// Start already reports frame 0 / 0 seconds, so there is nothing left
		// to say at this exact point.
		return;
	}

	float const secondsElapsed = (frame * frameSizeMS) / 1000.0f;

	SendSeqSyncPacket(fseq, frame, secondsElapsed);
	if (!media.empty())
	{
		SendMediaSyncPacket(media, secondsElapsed);
	}
}

void SyncManager::SendStop()
{
	if (!m_enabled || !m_sender.IsOpen())
	{
		return;
	}

	if (!m_lastFseq.empty())
	{
		SendSeqSyncStopPacket(m_lastFseq);
	}
	if (!m_lastMedia.empty())
	{
		SendMediaSyncStopPacket(m_lastMedia);
	}

	m_lastFseq.clear();
	m_lastMedia.clear();
	m_lastSyncedFrame = 0;
	m_lastMediaHalfSecond = 0;
}

void SyncManager::SendSeqOpenPacket(std::string const& filename)
{
	SendPacket(filename, SYNC_PKT_OPEN, SYNC_FILE_SEQ, 0, 0.0f);
}

void SyncManager::SendSeqSyncStartPacket(std::string const& filename)
{
	SendPacket(filename, SYNC_PKT_START, SYNC_FILE_SEQ, 0, 0.0f);
}

void SyncManager::SendSeqSyncStopPacket(std::string const& filename)
{
	SendPacket(filename, SYNC_PKT_STOP, SYNC_FILE_SEQ, 0, 0.0f);
}

void SyncManager::SendSeqSyncPacket(std::string const& filename, uint32_t frame, float secondsElapsed)
{
	uint32_t const diff = frame - m_lastSyncedFrame;
	if (frame > FPP_SEQ_SYNC_INITIAL_NUMBER_OF_FRAMES)
	{
		if (diff < FPP_SEQ_SYNC_INTERVAL_FRAMES)
		{
			return;
		}
	}
	else if (frame && diff < FPP_SEQ_SYNC_INTERVAL_INITIAL_FRAMES)
	{
		return;
	}

	SendPacket(filename, SYNC_PKT_SYNC, SYNC_FILE_SEQ, frame, secondsElapsed);
	m_lastSyncedFrame = frame;
}

void SyncManager::SendMediaOpenPacket(std::string const& filename)
{
	SendPacket(filename, SYNC_PKT_OPEN, SYNC_FILE_MEDIA, 0, 0.0f);
}

void SyncManager::SendMediaSyncStartPacket(std::string const& filename)
{
	SendPacket(filename, SYNC_PKT_START, SYNC_FILE_MEDIA, 0, 0.0f);
}

void SyncManager::SendMediaSyncStopPacket(std::string const& filename)
{
	SendPacket(filename, SYNC_PKT_STOP, SYNC_FILE_MEDIA, 0, 0.0f);
}

void SyncManager::SendMediaSyncPacket(std::string const& filename, float secondsElapsed)
{
	// Bucket comparison rather than a raw millisecond delta: this catches up
	// after a late tick instead of accumulating drift the way subtracting a
	// running total would.
	int const halfSecond = static_cast<int>(secondsElapsed * 2.0f);
	if (halfSecond == m_lastMediaHalfSecond)
	{
		return;
	}

	SendPacket(filename, SYNC_PKT_SYNC, SYNC_FILE_MEDIA, 0, secondsElapsed);
	m_lastMediaHalfSecond = halfSecond;
}

void SyncManager::SendPacket(std::string const& filename, uint8_t syncAction, uint8_t fileType,
	uint32_t frame, float secondsElapsed)
{
	// FPP matches on the bare file name, not the master's path to it.
	std::string const name = std::filesystem::path(filename).filename().string();

	size_t const bufsize = sizeof(ControlPkt) + sizeof(SyncPkt) + name.size();
	std::vector<uint8_t> buffer(bufsize, 0);

	ControlPkt* cp = reinterpret_cast<ControlPkt*>(buffer.data());
	memcpy(cp->fppd, "FPPD", 4);
	cp->pktType = CTRL_PKT_SYNC;
	cp->extraDataLen = static_cast<uint16_t>(bufsize - sizeof(ControlPkt));

	SyncPkt* sp = reinterpret_cast<SyncPkt*>(buffer.data() + sizeof(ControlPkt));
	sp->pktType = syncAction;
	sp->fileType = fileType;

	// Open/Start/Stop always report frame 0 / 0 seconds in real FPP; only a
	// genuine Sync packet carries live position data.
	if (syncAction == SYNC_PKT_SYNC)
	{
		sp->frameNumber = frame;
		sp->secondsElapsed = secondsElapsed;
	}
	else
	{
		sp->frameNumber = 0;
		sp->secondsElapsed = 0.0f;
	}

	// buffer was zero filled, so the name is already NUL terminated.
	memcpy(&sp->filename[0], name.c_str(), name.size());

	m_sender.Send(buffer.data(), bufsize);
}
