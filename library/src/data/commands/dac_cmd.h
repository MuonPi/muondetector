#ifndef DAC_REQUEST_CMD_H
#define DAC_REQUEST_CMD_H

#include <cstdint>

struct DacCmd {
    std::uint8_t channel;
    std::uint8_t value;
};

struct DacRequestCmd {};

#endif // DAC_REQUEST_CMD_H
