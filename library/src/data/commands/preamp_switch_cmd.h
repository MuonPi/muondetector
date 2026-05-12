#ifndef PREAMP_SWITCH_CMD_H
#define PREAMP_SWITCH_CMD_H

#include <cstdint>

struct PreampSwitchCmd {
    std::uint8_t channel{0};
    bool state{false};
};

struct PreampSwitchRequestCmd {};

#endif // PREAMP_SWITCH_CMD_H
