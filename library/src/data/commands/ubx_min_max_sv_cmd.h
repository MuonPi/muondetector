#ifndef UBX_MIN_MAX_SV_CMD_H
#define UBX_MIN_MAX_SV_CMD_H

#include <cstdint>

struct UbxMinMaxSvCmd
{
    std::uint8_t minSVs;
    std::uint8_t maxSVs;
};

#endif // UBX_MIN_MAX_SV_CMD_H
