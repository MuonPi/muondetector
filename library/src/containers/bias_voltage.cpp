#include "containers/bias_voltage.h"
#include <string>

bias_voltage::bias_voltage(float f_voltage)
    : m_voltage{f_voltage} {}

auto bias_voltage::byte_array() -> std::string
{
    return "";
}

auto bias_voltage::json() -> std::string
{
    return "";
}