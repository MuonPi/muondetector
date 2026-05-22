#ifndef UBX_VERSION_DEPENDENT_CMD_H
#define UBX_VERSION_DEPENDENT_CMD_H

#include "data/commands/ubx_gnss_config_cmd.h"
#include "data/commands/ubx_msg_rate_cmd.h"
#include "data/ublox/ublox_messages.h"

#include <compare>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

struct UbxVersionDependentCmd {
    struct Entry {
        UbxProtVersion min;
        UbxProtVersion max;
        std::variant<UbxMsgRateCmd, UbxGnssConfigCmd> cmd;
    };

    std::vector<Entry> config;

    inline static auto applies(const Entry& e, const UbxProtVersion& v) -> bool {
        return !(v < e.min) && !(e.max < v);
    }
};

#endif // UBX_VERSION_DEPENDENT_CMD_H