#ifndef UBX_GNSS_CONFIG_CMD_H
#define UBX_GNSS_CONFIG_CMD_H

#include "data/ublox/ublox_structs.h"

#include <vector>

struct UbxGnssConfigCmd {
    std::vector<GnssConfigStruct> gnssConfigs{};
};

#endif // UBX_GNSS_CONFIG_CMD_H