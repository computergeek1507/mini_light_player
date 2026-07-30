#ifndef RENARDOUTPUT_H
#define RENARDOUTPUT_H

#include "SerialOutput.h"

#include <vector>

// Plain async serial, no framing beyond a single sync byte and address - no
// break signal, unlike raw DMX.
//
// Byte layout and the value-substitution scheme below are confirmed against
// xLights' own src-core/outputs/RenardOutput.cpp (GetType() == "Renard")
// rather than the generic Renard protocol write-ups, which describe true
// byte-stuffing (escape byte + transformed value, expanding the frame by one
// byte per occurrence). xLights instead nudges the handful of reserved
// intensity values by 1 so they never appear in the data at all, which keeps
// the frame length fixed at channels + 2 - a receiver expecting real
// byte-stuffing would misinterpret a stray 0x7E as a new sync marker, so
// this project deliberately matches xLights' behaviour, not the spec.
//   [0]    0x7E         sync / start of message
//   [1]    0x80         board address (first/only board in the chain)
//   [2..]  channel data, with 0x7D/0x7E -> 0x7C and 0x7F -> 0x80
struct RenardOutput : SerialOutput
{
	RenardOutput();

	bool Open() override;
	void Close() override;
	void OutputFrame(uint8_t* data) override;

	// xLights: RENARD_MAX_CHANNELS.
	static constexpr uint32_t kMaxChannels = 1015;

	uint32_t BaudRate{ 57600 };

private:
	std::vector<uint8_t> m_data;
};

#endif
