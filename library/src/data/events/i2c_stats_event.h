#ifndef I2C_STATS_EVENT_H
#define I2C_STATS_EVENT_H

#include "data/muondetector_structs.h"

#include <cstdint>
#include <string>
#include <vector>

struct I2CStatsEvent {
    std::uint8_t nrDevices{0};
    std::uint32_t bytesRead{0};
    std::uint32_t bytesWritten{0};
    std::vector<I2cDeviceEntry> deviceList{};
};

#endif // I2C_STATS_EVENT_H
