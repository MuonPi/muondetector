#ifndef THRESHOLD_SETTING_CMD_H
#define THRESHOLD_SETTING_CMD_H

#include <cstdint>

struct ThresholdSettingCmd {
    std::uint8_t channel;
    float threshold;
};

#endif // THRESHOLD_SETTING_CMD_H