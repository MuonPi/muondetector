#ifndef PREAMP_SWITCH_CMD_H
#define PREAMP_SWITCH_CMD_H

#include <cstdint>

struct PreampSwitchRequestCmd {
    std::uint8_t channel{0};
};

#endif // PREAMP_SWITCH_CMD_H
