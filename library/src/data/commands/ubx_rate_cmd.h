#ifndef UBX_RATE_CMD_H
#define UBX_RATE_CMD_H

#include <cstdint>

struct UbxRateCmd {
    std::uint16_t measRate;
    std::uint16_t navRate;
};

#endif // UBX_RATE_CMD_H