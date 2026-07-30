#include "DMXOutput.h"

#include <algorithm>
#include <cstring>

DMXOutput::DMXOutput()
{
}

bool DMXOutput::Open()
{
	if (IP.empty() || !Enabled)
	{
		return false;
	}

	uint32_t const channels = static_cast<uint32_t>(std::min<uint64_t>(Channels, kMaxChannels));

	// header(4) + start code(1) + channels + footer(1)
	m_data.assign(channels + 6, 0);

	uint32_t const dataLength = channels + 1; // start code + channel bytes

	m_data[0] = 0x7E;
	m_data[1] = 6; // Output Only Send DMX Packet Request
	m_data[2] = static_cast<uint8_t>(dataLength & 0xFF);
	m_data[3] = static_cast<uint8_t>((dataLength >> 8) & 0xFF);
	m_data[4] = 0; // DMX start code
	m_data.back() = 0xE7;

	return OpenSerial(kBaudRate, false);
}

void DMXOutput::OutputFrame(uint8_t* data)
{
	if (!Enabled || !IsOpen() || m_data.empty())
	{
		return;
	}

	uint32_t const channels = static_cast<uint32_t>(m_data.size() - 6);
	memcpy(&m_data[5], &data[StartChannel - 1], channels);

	Send(m_data.data(), m_data.size());
}

void DMXOutput::Close()
{
	CloseSerial();
}
