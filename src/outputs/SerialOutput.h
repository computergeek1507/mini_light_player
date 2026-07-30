#ifndef SERIALOUTPUT_H
#define SERIALOUTPUT_H

#include "BaseOutput.h"
#include "SerialSender.h"

#include <cstdint>
#include <string>

// Mirrors IPOutput's role for the UDP outputs: shared open/close/send over
// the underlying transport, with each protocol responsible for its own
// frame format. BaseOutput::IP holds the OS port name here (e.g. "COM7" or
// "/dev/ttyUSB0"), reusing the field xLights itself stores it in via the
// network row's ComPort attribute.
struct SerialOutput : BaseOutput
{
	SerialOutput() = default;

	bool OpenSerial(uint32_t baudRate, bool twoStopBits)
	{
		return m_sender.Open(IP, baudRate, twoStopBits);
	}

	void CloseSerial() { m_sender.Close(); }

	bool Send(uint8_t const* data, std::size_t len) { return m_sender.Send(data, len); }

	[[nodiscard]] bool IsOpen() const { return m_sender.IsOpen(); }

	SerialSender m_sender;
};

#endif
