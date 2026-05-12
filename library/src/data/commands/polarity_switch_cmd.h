#ifndef POLARITY_SWITCH_CMD_H
#define POLARITY_SWITCH_CMD_H

#include <cstdint>

struct PolaritySwitchCmd {
    bool pol1{false};
    bool pol2{false};
};

struct PolaritySwitchRequestCmd {};

#endif // POLARITY_SWITCH_CMD_H
