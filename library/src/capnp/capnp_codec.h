#ifndef CAPNP_CODEC_H
#define CAPNP_CODEC_H

#include <cstdint>
#include <vector>

struct Ads1115Event;
struct NavSat;
struct GpioEvent;

template <typename T>
struct CapnpCodec {
    static auto encode(const T&) -> std::vector<std::uint8_t> {
        static_assert(sizeof(T) == 0, "No CapnpCodec specialization for this type");
        return {};
    }

    static auto decode(const std::vector<std::uint8_t>&) -> T {
        static_assert(sizeof(T) == 0, "No CapnpCodec specialization for this type");
        return {};
    }

    static auto messageKey() -> std::uint16_t {
        static_assert(sizeof(T) == 0, "No messageKey specialization for this type");
        return 0;
    }
};

template <>
struct CapnpCodec<Ads1115Event> {
    static auto encode(const Ads1115Event&) -> std::vector<std::uint8_t>;
    static auto decode(const std::vector<std::uint8_t>& data) -> Ads1115Event;
    static auto messageKey() -> std::uint16_t;
};

template <>
struct CapnpCodec<NavSat> {
    static auto encode(const NavSat&) -> std::vector<std::uint8_t>;
    static auto decode(const std::vector<std::uint8_t>& data) -> NavSat;
    static auto messageKey() -> std::uint16_t;
};

template <>
struct CapnpCodec<GpioEvent> {
    static auto encode(const GpioEvent&) -> std::vector<std::uint8_t>;
    static auto decode(const std::vector<std::uint8_t>& data) -> GpioEvent;
    static auto messageKey() -> std::uint16_t;
};
#endif // CAPNP_CODEC_H
