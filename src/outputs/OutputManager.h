#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H

#include "BaseOutput.h"

#include "spdlog/spdlog.h"

#include <memory>
#include <string>
#include <vector>

class OutputManager
{
public:

    OutputManager();
    bool LoadOutputs(std::string const& outputConfig);

    bool OpenOutputs();
    void CloseOutputs();
    void OutputData(uint8_t* data);
    uint64_t getChannelCount()const{ return m_channelCount;}

private:
    std::vector<std::unique_ptr<BaseOutput>> m_outputs;
    std::shared_ptr<spdlog::logger> m_logger{ nullptr };

    uint64_t m_channelCount{0};
};

#endif