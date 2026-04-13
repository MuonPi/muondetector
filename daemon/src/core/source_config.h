#ifndef SOURCE_CONFIG_H
#define SOURCE_CONFIG_H

#include "sources/source.h"
#include "hardware/devices.h"

#include <string>
#include <optional>


struct SourceConfig
{
    SourceId id;
    std::optional<Device> deviceId{std::nullopt};
    std::optional<std::chrono::milliseconds> interval{std::nullopt};
};

#endif // SOURCE_CONFIG_H