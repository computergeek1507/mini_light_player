#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include "UdpSender.h"

#include "spdlog/spdlog.h"

#include <cstdint>
#include <string>

#include <memory>
#include <vector>

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
	void SendFPPSync(const std::string& item, uint32_t stepMS, uint32_t frames);

	// Off by default: a player that is not driving remote FPP instances should
	// not be putting sync traffic on the network.
	bool m_enabled{ false };
	std::string m_lastFseq;
	std::string m_lastMedia;
	uint32_t m_lastFrame{ 0 };
	uint32_t m_lastMediaMsec{ 0 };
	UdpSender m_sender;
	std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};

#endif