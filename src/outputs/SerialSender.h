#ifndef SERIALSENDER_H
#define SERIALSENDER_H

#include <cstdint>
#include <string>

// Opens a serial port for write-only output: 8 data bits, no parity, and
// either 1 or 2 stop bits at a fixed baud rate.
//
// No break-signal support is needed here. DMXOutput uses the Enttec USB Pro
// framed protocol (a header/length/footer wrapper the dongle's own firmware
// unwraps into real DMX512), and RenardOutput is a plain async byte stream -
// neither requires the host to generate a manual DMX break. Confirmed against
// xLights' own src-core/outputs/{DMXOutput,RenardOutput,serial}.cpp: their
// break-generating SerialPort::SendBreak() is only ever called by
// OpenDMXOutput, a different class this project does not implement.
class SerialSender
{
public:
	SerialSender() = default;
	~SerialSender() { Close(); }

	SerialSender(SerialSender const&) = delete;
	SerialSender& operator=(SerialSender const&) = delete;

	bool Open(std::string const& portName, uint32_t baudRate, bool twoStopBits);
	void Close();

	[[nodiscard]] bool IsOpen() const;

	bool Send(uint8_t const* data, std::size_t len);

private:
#ifdef _WIN32
	// HANDLE, kept as void* so <windows.h> does not leak into every includer.
	void* m_handle{ nullptr };
#else
	int m_fd{ -1 };
#endif
};

#endif
