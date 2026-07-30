#include "RenardOutput.h"

#include <algorithm>

namespace
{
	uint8_t Sanitize(uint8_t value)
	{
		if (value == 0x7D || value == 0x7E)
		{
			return 0x7C;
		}
		if (value == 0x7F)
		{
			return 0x80;
		}
		return value;
	}
}

RenardOutput::RenardOutput()
{
}

bool RenardOutput::Open()
{
	if (IP.empty() || !Enabled)
	{
		return false;
	}

	uint32_t const channels = static_cast<uint32_t>(std::min<uint64_t>(Channels, kMaxChannels));

	m_data.assign(channels + 2, 0);
	m_data[0] = 0x7E;
	m_data[1] = 0x80;

	// xLights: "use 2 stop bits so padding chars are not required".
	return OpenSerial(BaudRate, true);
}

void RenardOutput::OutputFrame(uint8_t* data)
{
	if (!Enabled || !IsOpen() || m_data.empty())
	{
		return;
	}

	uint32_t const channels = static_cast<uint32_t>(m_data.size() - 2);
	for (uint32_t i = 0; i < channels; ++i)
	{
		m_data[2 + i] = Sanitize(data[StartChannel - 1 + i]);
	}

	Send(m_data.data(), m_data.size());
}

void RenardOutput::Close()
{
	CloseSerial();
}
