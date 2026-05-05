#ifndef CALIB_EVENT_H
#define CALIB_EVENT_H

#include <cstdint>
#include <data/muondetector_structs.h>
#include <vector>

struct CalibEvent {
    bool valid{false};
    bool eepromValid{false};
    std::uint64_t id{0};
    std::vector<CalibStruct> calibList{};
};

#endif // CALIB_EVENT_H