#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include "containers/message_container.h"
#include <map>

class gpio_config : public message_container
{
public:
    enum class gpio_direction {
        input,
        output
    };
    enum class gpio_additional {
        none,
        pullup,
        pulldown
    };
    using gpio_config_map = std::map<unsigned, std::pair<gpio_direction, gpio_additional>>;
    gpio_config(const gpio_config_map &f_gpio_map);
    auto byte_array() -> std::string override;
    auto json() -> std::string override;

    auto static constexpr output = std::pair<gpio_config::gpio_direction, gpio_config::gpio_additional> {gpio_config::gpio_direction::output, gpio_config::gpio_additional::none};
    auto static constexpr pullup = std::pair<gpio_config::gpio_direction, gpio_config::gpio_additional>{gpio_config::gpio_direction::input, gpio_config::gpio_additional::pullup};
    auto static constexpr input = std::pair<gpio_config::gpio_direction, gpio_config::gpio_additional>{gpio_config::gpio_direction::input, gpio_config::gpio_additional::none};

    void emplace(unsigned key, std::pair<gpio_config::gpio_direction, gpio_config::gpio_additional> val);
    auto get_map() -> const gpio_config_map &;

private:
    gpio_config_map m_gpio_map;
};

#endif // GPIO_CONFIG_H