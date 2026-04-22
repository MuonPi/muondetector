#ifndef UBX_SET_AOP_CMD_H
#define UBX_SET_AOP_CMD_H

#include <cstdint>

struct UbxSetAopCmd {
    bool enable;
    std::uint16_t maxOrbErr{0}; // Defaults to 0
};

#endif // UBX_SET_AOP_CMD_H