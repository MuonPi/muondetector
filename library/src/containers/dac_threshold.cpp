#include "containers/dac_threshold.h"
#include <string>

dac_threshold::dac_threshold(uint8_t f_channel, float f_threshold)
    : m_channel{f_channel}
    , m_threshold{f_threshold} {}

auto dac_threshold::byte_array() -> std::string
{
    return "";
}

auto dac_threshold::json() -> std::string
{
    return "";
}