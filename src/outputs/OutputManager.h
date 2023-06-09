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


private:
    std::vector<std::unique_ptr<BaseOutput>> m_outputs;
    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};

#endif