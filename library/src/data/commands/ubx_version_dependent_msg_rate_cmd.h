#ifndef UBX_VERSION_DEPENDENT_MSG_RATE_CMD_H
#define UBX_VERSION_DEPENDENT_MSG_RATE_CMD_H

#include "data/commands/ubx_msg_rate_cmd.h"
#include "data/ublox/ublox_messages.h"

#include <compare>
#include <optional>
#include <stdexcept>
#include <vector>

struct UbxVersionDependentMsgRateCmd {
    struct Entry {
        Version min;
        Version max;
        UbxMsgRateCmd cmd;
    };

    std::vector<Entry> config;

    inline static auto applies(const Entry& e, const Version& v) -> bool {
        return !(v < e.min) && !(e.max < v);
    }
};

#endif