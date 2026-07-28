#ifndef IPOUTPUT_H
#define IPOUTPUT_H

#include "BaseOutput.h"

#include "spdlog/spdlog.h"

#include <MinimalSocket/udp/UdpSocket.h>

#include <cstdint>
#include <memory>
#include <optional>

struct IPOutput : BaseOutput
{
	IPOutput() = default;

	// Binds an ephemeral local port and connects the socket to remoteIp:remotePort.
	// The destination port must not be reused as the local port, otherwise a
	// second output speaking the same protocol fails to bind.
	bool OpenSocket(std::string const& remoteIp, MinimalSocket::Port remotePort)
	{
		auto logger = spdlog::get("miniplayer");

		// Not isValidHost(): v.3.1 declares it in the header but defines
		// isValidAddress() in the .cpp, so it does not link.
		if (MinimalSocket::deduceAddressFamily(remoteIp) == std::nullopt)
		{
			if (logger) logger->error("Invalid output address: {}:{}", remoteIp, remotePort);
			return false;
		}

		MinimalSocket::Address const remote(remoteIp, remotePort);

		// ANY_PORT lets the OS pick the local port so outputs can share a protocol.
		auto socket = std::make_unique<MinimalSocket::udp::UdpConnected<true>>(remote, MinimalSocket::ANY_PORT);

		if (!socket->open())
		{
			if (logger) logger->error("Unable to open socket for {}:{}", remoteIp, remotePort);
			return false;
		}

		m_UdpSocket = std::move(socket);
		return true;
	}

	void CloseSocket() { m_UdpSocket.reset(); }

	// Sends len raw bytes. Never pass packet data through the std::string
	// overload of MinimalSocket::send - it would stop at the first NUL byte.
	bool Send(uint8_t const* data, std::size_t len)
	{
		if (m_UdpSocket == nullptr)
		{
			return false;
		}
		return m_UdpSocket->send(MinimalSocket::BufferViewConst{ reinterpret_cast<char const*>(data), len });
	}

	std::unique_ptr<MinimalSocket::udp::UdpConnected<true>> m_UdpSocket;
};

#endif
