#ifndef PCA_SWITCH_EVENT_H
#define PCA_SWITCH_EVENT_H

#include <cstdint>

struct PcaSwitchEvent {
    std::uint8_t pcaPortMask{0};
};

#endif // PCA_SWITCH_EVENT_H