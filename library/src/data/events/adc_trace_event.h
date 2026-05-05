#ifndef ADC_TRACE_EVENT_H
#define ADC_TRACE_EVENT_H

#include <vector>

struct AdcTraceEvent {
    std::vector<float> adcSampleBuffer{};
};

#endif // ADC_TRACE_EVENT_H