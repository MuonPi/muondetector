#include "capnp/capnp_codec.h"

#include "ads1115.capnp.h"
#include "data/events/ads1115_event.h"
#include "data/events/gpio_event.h"
#include "data/events/ubx_event.h"
#include "gpio_event.capnp.h"
#include "nav_sat.capnp.h"
#include "network/tcpmessage_keys.h"

#include <capnp/serialize.h>
#include <cstdint>
#include <vector>

inline capnp::FlatArrayMessageReader makeReader(const std::vector<std::uint8_t>& data) {
    auto wordPtr = reinterpret_cast<const capnp::word*>(data.data());
    auto wordCount = data.size() / sizeof(capnp::word);

    return capnp::FlatArrayMessageReader(kj::ArrayPtr<const capnp::word>(wordPtr, wordCount));
}

auto CapnpCodec<Ads1115Event>::encode(const Ads1115Event& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<Ads1115EventCapnp>();

    root.setDeviceId(event.deviceId);
    root.setChannel(event.channel);
    root.setRawValue(event.rawValue);
    root.setVoltage(event.voltage);
    root.setTimestamp(event.timestamp);

    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();

    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}

auto CapnpCodec<Ads1115Event>::decode(const std::vector<std::uint8_t>& data) -> Ads1115Event {
    auto reader = makeReader(data);
    auto root = reader.getRoot<Ads1115EventCapnp>();

    Ads1115Event event;
    event.deviceId = root.getDeviceId();
    event.channel = root.getChannel();
    event.rawValue = root.getRawValue();
    event.voltage = root.getVoltage();
    event.timestamp = root.getTimestamp();

    return event;
}

auto CapnpCodec<Ads1115Event>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_ADC_SAMPLE);
}

auto CapnpCodec<NavSat>::encode(const NavSat& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<NavSatCapnp>();

    root.setITOW(event.iTOW);
    root.setHasVersion(event.version.has_value());
    if (event.version.has_value()) {
        root.setVersion(event.version.value());
    }
    root.setHasGlobFlags(event.globFlags.has_value());
    if (event.globFlags.has_value()) {
        root.setGlobFlags(event.globFlags.value());
    }
    root.setNumSvs(event.numSvs);
    root.setGoodSats(event.goodSats);

    /// TODO: Add Satellites

    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();

    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}

auto CapnpCodec<NavSat>::decode(const std::vector<std::uint8_t>& data) -> NavSat {
    auto reader = makeReader(data);
    auto root = reader.getRoot<NavSatCapnp>();

    NavSat event;

    event.iTOW = root.getITOW();

    if (root.getHasVersion()) {
        event.version = root.getVersion();
    } else {
        event.version = std::nullopt;
    }

    if (root.getHasGlobFlags()) {
        event.globFlags = root.getGlobFlags();
    } else {
        event.globFlags = std::nullopt;
    }

    event.numSvs = root.getNumSvs();
    event.goodSats = root.getGoodSats();

    // TODO: satellites if needed

    return event;
}

auto CapnpCodec<NavSat>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GNSS_SATS);
}

auto CapnpCodec<GpioEvent>::encode(const GpioEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GpioEventCapnp>();

    root.setSig(static_cast<std::uint8_t>(event.gpio_signal));
    root.setPin(static_cast<std::uint8_t>(event.gpio_pin));
    root.setTimestamp(static_cast<std::uint8_t>(event.timestamp.count()));
    root.setEdge(static_cast<std::uint8_t>(event.edge));

    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();

    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}

auto CapnpCodec<GpioEvent>::decode(const std::vector<std::uint8_t>& data) -> GpioEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GpioEventCapnp>();

    GpioEvent event;

    event.gpio_signal = static_cast<GPIO_SIGNAL>(root.getSig());
    event.gpio_pin = static_cast<unsigned int>(root.getPin());

    // WARNING: currently lossy encoding
    event.timestamp = std::chrono::nanoseconds(root.getTimestamp());

    event.edge = static_cast<EventEdge>(root.getEdge());

    return event;
}

auto CapnpCodec<GpioEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GPIO_EVENT);
}
