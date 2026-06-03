#ifndef SDS011_EVENT_H
#define SDS011_EVENT_H

#include <cstdint>
#include <optional>
#include <string>

struct Sds011Event {
    std::uint16_t pm2dot5{0};
    std::uint16_t pm10dot0{0};
    std::uint16_t id{0};
};

struct Sds011StatusEvent {
    std::optional<std::uint8_t> ackType;
    std::optional<bool> sleep;
    std::optional<bool> queryOnlyMode;
    std::optional<std::uint8_t> modeByte;
    std::optional<std::uint16_t> id;
    std::optional<std::string> firmwareDate;
};
#endif // SDS011_EVENT_H