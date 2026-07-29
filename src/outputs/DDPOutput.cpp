#include "DDPOutput.h"

#include <cstring>

DDPOutput::DDPOutput()
{
	memset(_data, 0, sizeof(_data));
}

bool DDPOutput::Open()
{
	if (IP.empty() || !Enabled) return false;

	memset(_data, 0x00, sizeof(_data));

    _data[2] = 1;
    _data[3] = DDP_ID_DISPLAY;
    _sequenceNum = 1;

    return OpenSocket(IP, DDP_PORT);
}

void DDPOutput::OutputFrame(uint8_t* data)
{
    if (!Enabled || !IsOpen()) return;

    int32_t index = StartChannel - 1;
    int32_t chan = KeepChannels ? (StartChannel - 1) : 0;
    int32_t tosend = Channels;

    while (tosend > 0)
    {
        int32_t thissend = (tosend < PacketSize) ? tosend : PacketSize;

        if (tosend == thissend) {
            _data[0] = DDP_FLAGS1_VER1 | DDP_FLAGS1_PUSH;
        }
        else {
            _data[0] = DDP_FLAGS1_VER1;
        }

        _data[1] = (_data[1] & 0xF0) + _sequenceNum;

        _data[4] = (chan & 0xFF000000) >> 24;
        _data[5] = (chan & 0xFF0000) >> 16;
        _data[6] = (chan & 0xFF00) >> 8;
        _data[7] = (chan & 0xFF);

        _data[8] = (thissend & 0xFF00) >> 8;
        _data[9] = thissend & 0x00FF;

        memcpy(&_data[DDP_PACKET_HEADERLEN], data + index, thissend);

        Send(_data, DDP_PACKET_HEADERLEN + thissend);
        _sequenceNum = _sequenceNum == 15 ? 1 : _sequenceNum + 1;

        tosend -= thissend;
        index += thissend;
        chan += thissend;
    }
}

void DDPOutput::Close()
{
	CloseSocket();
}