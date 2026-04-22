#ifndef ADS1115_EVENT_H
#define ADS1115_EVENT_H

#include <cstdint>

struct Ads1115Event {
    std::uint32_t deviceId;
    std::uint8_t channel;
    std::uint16_t rawValue;
    float voltage;
    std::uint64_t timestamp;
};

#endif // ADS1115_EVENT_H
