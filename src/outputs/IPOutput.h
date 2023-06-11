#ifndef IPOUTPUT_H
#define IPOUTPUT_H

#include "BaseOutput.h"

#include <MinimalSocket/udp/UdpSocket.h>

#include <memory>

struct IPOutput : BaseOutput
{
	IPOutput() {}
	std::unique_ptr<MinimalSocket::udp::UdpBinded> m_UdpSocket;
};

#endif