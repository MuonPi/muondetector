#ifndef ADS1115_EVENT_H
#define ADS1115_EVENT_H

#include <cstdint>

struct ADS1115Event {
    std::uint32_t deviceId;
    std::uint8_t channel;
    std::uint16_t rawValue;
    float voltage;
    std::uint64_t timestamp;
    double convTime;
    // ADC_SAMPLING_MODE samplingMode{ADC_SAMPLING_MODE::PEAK};
};

#endif // ADS1115_EVENT_H
