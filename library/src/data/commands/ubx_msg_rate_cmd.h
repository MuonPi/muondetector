#ifndef UBX_MSG_RATE_CMD_H
#define UBX_MSG_RATE_CMD_H

#include "data/ublox/ublox_messages.h"

struct UbxMsgRateCmd {
    UBX_MSG::msg_id id;
    std::uint8_t port;
    std::uint8_t rate;
};

struct UbxMsgRateRequestCmd {};

#endif // UBX_MSG_RATE_CMD_H