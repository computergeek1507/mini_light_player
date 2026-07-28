#ifndef UDPSENDER_H
#define UDPSENDER_H

#include "spdlog/spdlog.h"

#include <MinimalSocket/udp/UdpSocket.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

// A connected UDP socket bound to an ephemeral local port.
//
// Binding the destination port locally would stop a second sender of the same
// protocol from starting, and MinimalSocket's std::string send overload stops
// at the first NUL byte, so packet data must go through a buffer view.
class UdpSender
{
public:
	bool Open(std::string const& remoteIp, MinimalSocket::Port remotePort)
	{
		auto logger = spdlog::get("miniplayer");

		// Not isValidHost(): v.3.1 declares it in the header but defines
		// isValidAddress() in the .cpp, so it does not link.
		if (MinimalSocket::deduceAddressFamily(remoteIp) == std::nullopt)
		{
			if (logger) logger->error("Invalid address: {}:{}", remoteIp, remotePort);
			return false;
		}

		MinimalSocket::Address const remote(remoteIp, remotePort);
		auto socket = std::make_unique<MinimalSocket::udp::UdpConnected<true>>(remote, MinimalSocket::ANY_PORT);

		if (!socket->open())
		{
			if (logger) logger->error("Unable to open socket for {}:{}", remoteIp, remotePort);
			return false;
		}

		m_socket = std::move(socket);
		return true;
	}

	void Close() { m_socket.reset(); }

	[[nodiscard]] bool IsOpen() const { return m_socket != nullptr; }

	bool Send(uint8_t const* data, std::size_t len)
	{
		if (m_socket == nullptr)
		{
			return false;
		}
		return m_socket->send(MinimalSocket::BufferViewConst{ reinterpret_cast<char const*>(data), len });
	}

private:
	std::unique_ptr<MinimalSocket::udp::UdpConnected<true>> m_socket;
};

#endif
