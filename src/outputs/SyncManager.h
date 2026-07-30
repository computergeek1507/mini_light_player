#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include "UdpSender.h"

#include "spdlog/spdlog.h"

#include <cstdint>
#include <string>

#include <memory>
#include <vector>

// Sends the same multisync wire protocol a real FPP master uses: an Open
// packet followed by a Start when a file begins, throttled Sync packets
// while it plays, and a Stop when it ends - independently for the sequence
// and the media file. See SyncManager.cpp for exactly where each behaviour
// was confirmed against FalconChristmas/fpp's own source.
class SyncManager {

public:

	SyncManager();
	~SyncManager();

	bool OpenOutputs();
	void CloseOutputs();

	void SendStop();
	void SendSync(uint32_t frameSizeMS, uint32_t frame, std::string const& fseq, std::string const& media);

	bool IsEnabled() const { return m_enabled; }
	void SetEnabled(bool enable)
	{
		m_enabled = enable;
	}

private:
	// Mirrors FPP's own MultiSync API shape: separate Open/Start/Stop/Sync
	// calls per content type, rather than one generic "send something".
	void SendSeqOpenPacket(std::string const& filename);
	void SendSeqSyncStartPacket(std::string const& filename);
	void SendSeqSyncStopPacket(std::string const& filename);
	void SendSeqSyncPacket(std::string const& filename, uint32_t frame, float secondsElapsed);

	void SendMediaOpenPacket(std::string const& filename);
	void SendMediaSyncStartPacket(std::string const& filename);
	void SendMediaSyncStopPacket(std::string const& filename);
	void SendMediaSyncPacket(std::string const& filename, float secondsElapsed);

	void SendPacket(std::string const& filename, uint8_t syncAction, uint8_t fileType,
		uint32_t frame, float secondsElapsed);

	// Off by default: a player that is not driving remote FPP instances should
	// not be putting sync traffic on the network.
	bool m_enabled{ false };

	std::string m_lastFseq;
	std::string m_lastMedia;

	// Frame of the last Sync packet actually sent for the sequence, so the
	// throttle can measure how far playback has moved since then.
	uint32_t m_lastSyncedFrame{ 0 };

	// Bucketed to a half second, matching FPP's own comparison rather than a
	// raw millisecond difference. Reset to 0 whenever new media starts, since
	// the Start packet already covers that first bucket.
	int m_lastMediaHalfSecond{ 0 };

	UdpSender m_sender;
	std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};

#endif
