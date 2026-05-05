#ifndef ADC_SAMPLE_REQUEST_CMD_H
#define ADC_SAMPLE_REQUEST_CMD_H

#include <cstdint>

struct AdcSampleRequestCmd {
    std::uint8_t channel{0};
};

#endif // ADC_SAMPLE_REQUEST_CMD_H
