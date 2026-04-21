#ifndef UBX_SAVE_CMD_H
#define UBX_SAVE_CMD_H

#include "data/ublox/ublox_messages.h"
#include <cstdint>

struct UbxSaveCmd
{
    std::uint32_t devMask{static_cast<std::uint32_t>(UBX_DEV::DEV_BBR | UBX_DEV::DEV_FLASH)};
};

#endif // UBX_SAVE_CMD_H