#ifndef THRESHOLD_SETTING_EVENT_H
#define THRESHOLD_SETTING_EVENT_H

#include <cstdint>

struct ThresholdSettingEvent {
    std::uint8_t channel;
    double voltage;
    bool success;
};

#endif // THRESHOLD_SETTING_EVENT_H