#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include "spdlog/spdlog.h"


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

	bool m_enabled{ true };
	std::string m_lastFseq;
	std::string m_lastMedia;
	size_t m_lastFrame{ 0 };
	size_t m_lastMediaMsec{ 0 };
	//QStringList m_unicast;
	//QHostAddress m_groupAddress;
	//std::unique_ptr<QUdpSocket> m_UdpSocket{ nullptr };
	std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};

#endif