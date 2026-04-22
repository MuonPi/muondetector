#ifndef UBX_RESET_CMD_H
#define UBX_RESET_CMD_H

#include "data/ublox/ublox_messages.h"

#include <cstdint>

struct UbxResetCmd {
    std::uint32_t resetFlags{
        static_cast<std::uint32_t>(UBX_RESET::RESET_WARM | UBX_RESET::RESET_SW)};
};

#endif // UBX_RESET_CMD_H