#ifndef PCA_SWITCH_CMD_H
#define PCA_SWITCH_CMD_H

#include <cstdint>

struct PcaSwitchCmd {
    std::uint8_t pcaPortMask{0};
};

struct PcaSwitchRequestCmd {};

#endif // PCA_SWITCH_CMD_H
