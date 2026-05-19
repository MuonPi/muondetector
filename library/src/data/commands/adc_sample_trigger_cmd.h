#ifndef ADC_SAMPLE_TRIGGER_CMD_H
#define ADC_SAMPLE_TRIGGER_CMD_H

#include <cstdint>

struct AdcSampleTriggerCmd {
    std::uint8_t channel{0};
};

#endif // ADC_SAMPLE_TRIGGER_CMD_H
