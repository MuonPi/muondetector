#ifndef DAC_REQUEST_CMD_H
#define DAC_REQUEST_CMD_H

#include <cstdint>

struct DacRequestCmd {
    std::uint8_t channel{0};
};

#endif // DAC_REQUEST_CMD_H
