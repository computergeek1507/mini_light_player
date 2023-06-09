#ifndef IPOUTPUT_H
#define IPOUTPUT_H

#include "BaseOutput.h"


#include <memory>

struct IPOutput : BaseOutput
{
	IPOutput(asio::io_context io):m_UdpSocket(std::make_unique<udp::socket>(io)) {}
	std::unique_ptr<asio::ip::udp::socket> m_UdpSocket;
};

#endif