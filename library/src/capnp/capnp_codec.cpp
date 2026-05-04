#include "capnp/capnp_codec.h"

#include "ads1115.capnp.h"
#include "data/events/ads1115_event.h"
#include "data/events/ubx_event.h"
#include "nav_sat.capnp.h"
#include "network/tcpmessage_keys.h"

#include <capnp/serialize.h>
#include <cstdint>
#include <vector>

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

auto CapnpCodec<NavSat>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GNSS_SATS);
}
