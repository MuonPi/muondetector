#ifndef GPIO_STATE_H
#define GPIO_STATE_H

#include "containers/message_container.h"
#include <map>

class gpio_state : public message_container
{
public:
    gpio_state(std::pair<unsigned, bool> f_state);
    gpio_state(unsigned f_gpio, bool f_high);
    auto byte_array() -> std::string override;
    auto json() -> std::string override;
    auto gpio() const -> unsigned;
    auto high() const -> bool;
private:
    unsigned m_gpio;
    unsigned m_high;
};

#endif // GPIO_STATE_H
