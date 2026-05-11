#ifndef OLED_EVENT_H
#define OLED_EVENT_H

#include <string>

struct OledEvent {
    float andRate{0.};
    float xorRate{0.};
    float temp{0.};
    std::size_t nSats{0};
    std::size_t nSatsVisible{0};
    Gnss::FixType gpsFix{Gnss::FixType::None};
};

#endif // OLED_EVENT_H