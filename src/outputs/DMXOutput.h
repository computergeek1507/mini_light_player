#ifndef DMXOUTPUT_H
#define DMXOUTPUT_H

#include "SerialOutput.h"

#include <vector>

// The Enttec USB Pro "Output Only Send DMX Packet" framed protocol: a
// header/length/footer wrapper that the dongle's own firmware unwraps into
// real DMX512 (break, mark-after-break, start code, up to 512 channels).
// The host never generates a break itself.
//
// Byte layout, confirmed against xLights' own src-core/outputs/DMXOutput.cpp
// (GetType() == "DMX", matching the NetworkType xLights writes for this
// hardware) rather than assumed from the generic DMX512 spec:
//   [0]      0x7E                 start of message
//   [1]      6                    label: Output Only Send DMX Packet Request
//   [2..3]   data length, LSB/MSB (= 1 start code byte + channel count)
//   [4]      0x00                 DMX start code
//   [5..]    channel data
//   [last]   0xE7                 end of message
//
// xLights supports "Entec Pro, Lynx DMX, DIYC RPM, DMXking.com and
// DIYblinky.com" dongles this way, all of which speak this exact framing.
struct DMXOutput : SerialOutput
{
	DMXOutput();

	bool Open() override;
	void Close() override;
	void OutputFrame(uint8_t* data) override;

	// xLights: AllowsBaudRateSetting() == false - the framing protocol runs
	// over the dongle's virtual COM port at this fixed rate regardless of
	// what a network row's BaudRate attribute says.
	static constexpr uint32_t kBaudRate = 250000;

	// xLights: DMX_MAX_CHANNELS. Standard DMX512 caps at 512; the higher
	// figure covers non-standard multi-port dongles (e.g. DIYblinky) that
	// accept more in a single framed packet.
	static constexpr uint32_t kMaxChannels = 4800;

private:
	std::vector<uint8_t> m_data;
};

#endif
