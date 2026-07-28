#include "ArtNetOutput.h"

#include "spdlog/spdlog.h"

#include <memory>

ArtNetOutput::ArtNetOutput()
{
	memset(_data, 0, sizeof(_data));
}

bool ArtNetOutput::Open()
{
	if (IP.empty() || !Enabled) return false;

    memset(_data, 0x00, sizeof(_data));
    _sequenceNum = 1;

    _data[0] = 'A';   // ID[8]
    _data[1] = 'r';
    _data[2] = 't';
    _data[3] = '-';
    _data[4] = 'N';
    _data[5] = 'e';
    _data[6] = 't';
    _data[9] = 0x50;
    _data[11] = 0x0E; // Protocol version Low
    _data[14] = (Universe & 0xFF);
    _data[15] = ((Universe & 0xFF00) >> 8);
    _data[16] = 0x02; // we are going to send all 512 bytes

    if (IP == "MULTICAST") {
        // Art-Net discovery uses broadcast, which needs SO_BROADCAST on the socket.
        // Not reachable through MinimalSocket, so refuse rather than open a dead output.
        auto logger = spdlog::get("miniplayer");
        if (logger) logger->warn("ArtNet broadcast output is not supported, skipping universe {}", Universe);
        return false;
    }

    if (!OpenSocket(IP, ARTNET_PORT))
    {
        return false;
    }

    _data[16] = (uint8_t)(PacketSize >> 8);  // Length (high)
    _data[17] = (uint8_t)(PacketSize & 0xff);  // Length (low)

    return true;
}

void ArtNetOutput::OutputFrame(uint8_t* data)
{
    if (!Enabled || !IsOpen()) return;

    size_t const chs = PacketSize;
    memcpy(&_data[ARTNET_PACKET_HEADERLEN], &data[StartChannel - 1], chs);

    // Sequence 0 means "disable ordering", so the counter wraps 1..255.
    _data[12] = _sequenceNum;
    _sequenceNum = (_sequenceNum == 255) ? 1 : _sequenceNum + 1;

    Send(_data, ARTNET_PACKET_HEADERLEN + chs);
}

void ArtNetOutput::Close()
{
    CloseSocket();
}