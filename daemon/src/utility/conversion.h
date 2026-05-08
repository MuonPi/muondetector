#ifndef CONVERSION_H
#define CONVERSION_H

#include <string>

template <typename T>
std::string to_hex(T value) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << std::setw(sizeof(T) * 2) << std::setfill('0')
        << static_cast<uint64_t>(value);
    return oss.str();
}

#endif // CONVERSION_H