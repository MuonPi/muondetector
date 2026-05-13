#ifndef UBX_TP5_CMD_H
#define UBX_TP5_CMD_H

#include "data/events/ubx_event.h"

struct UbxTp5Cmd : UbxTimePulseStruct {
    UbxTp5Cmd(const UbxTimePulseStruct& tp5) : UbxTimePulseStruct(tp5) {}
};

#endif // UBX_TP5_CMD_H