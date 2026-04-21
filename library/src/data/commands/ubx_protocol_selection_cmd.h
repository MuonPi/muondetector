#ifndef UBX_PROTOCOL_SELECTION_CMD_H
#define UBX_PROTOCOL_SELECTION_CMD_H

#include <cstdint>

struct UbxProtocolSelectionCmd
{
    std::uint8_t port;
    std::uint8_t outProtocolMask;
};

#endif // UBX_PROTOCOL_SELECTION_CMD_H