#ifndef IPOUTPUT_H
#define IPOUTPUT_H

#include "BaseOutput.h"
#include "UdpSender.h"

#include <cstdint>
#include <string>

struct IPOutput : BaseOutput
{
	IPOutput() = default;

	bool OpenSocket(std::string const& remoteIp, MinimalSocket::Port remotePort)
	{
		return m_sender.Open(remoteIp, remotePort);
	}

	void CloseSocket() { m_sender.Close(); }

	bool Send(uint8_t const* data, std::size_t len) { return m_sender.Send(data, len); }

	// Kept for the null checks in the OutputFrame implementations.
	[[nodiscard]] bool IsOpen() const { return m_sender.IsOpen(); }

	UdpSender m_sender;
};

#endif
