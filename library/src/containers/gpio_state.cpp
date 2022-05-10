#include "containers/gpio_state.h"

gpio_state::gpio_state(std::pair<unsigned, bool> f_state)
    : m_gpio{f_state.first}
    , m_high{f_state.second} {}

gpio_state::gpio_state(unsigned f_gpio, bool f_high)
    : m_gpio{f_gpio}
    , m_high{f_high} {}

auto gpio_state::byte_array() -> std::string
{
    return "";
}

auto gpio_state::json() -> std::string
{
    return "";
}

auto gpio_state::gpio() const -> unsigned
{
    return m_gpio;
}

auto gpio_state::high() const -> bool
{
    return m_high;
}
