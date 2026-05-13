#ifndef POSITION_MODE_CMD_H
#define POSITION_MODE_CMD_H

#include "data/muondetector_structs.h"

struct PositionModeCmd : PositionModeConfig {
    PositionModeCmd(const PositionModeConfig& cfg) : PositionModeConfig(cfg) {}
};

#endif // POSITION_MODE_CMD_H