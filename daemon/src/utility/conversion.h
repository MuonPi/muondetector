#ifndef CONVERSION_H
#define CONVERSION_H

#include <iomanip>
#include <ostream>
#include <string>

template <typename T>
inline std::string to_hex(T value) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << std::setw(sizeof(T) * 2) << std::setfill('0')
        << static_cast<uint64_t>(value);
    return oss.str();
}

inline std::string dateStringNow() {
    auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());

    return std::format("{:%Y-%m-%d_%H-%M-%S}", now);
}

#endif // CONVERSION_H