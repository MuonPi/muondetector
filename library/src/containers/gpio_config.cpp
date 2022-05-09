#include "containers/gpio_config.h"
#include <string>

gpio_config::gpio_config(const gpio_config_map &f_gpio_map)
    : m_gpio_map{std::move(f_gpio_map)} {}

auto gpio_config::byte_array() -> std::string {
    return "";
}

auto gpio_config::json()->std::string
{
    return "";
}

void gpio_config::emplace(unsigned key, std::pair<gpio_config::gpio_direction, gpio_config::gpio_additional> val)
{
    m_gpio_map.emplace(key, std::move(val));
}

auto gpio_config::get_map() -> const gpio_config_map &
{
    return m_gpio_map;
}