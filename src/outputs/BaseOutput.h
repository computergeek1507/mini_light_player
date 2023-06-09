#ifndef BASEOUTPUT_H
#define BASEOUTPUT_H

#include <memory>
#include <string>

struct BaseOutput
{
	virtual bool Open() = 0;
	virtual void OutputFrame(uint8_t *data) = 0;
	virtual void Close() = 0;

	uint64_t StartChannel{0};
	uint64_t Channels{0};
	std::string IP;
	bool Enabled{true};
};

#endif