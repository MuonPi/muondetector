
#include "drivers/ads1115_driver.h"

#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "data/events/ad1115_event.h"
#include "hardware/devices.h"
#include "hardware/i2c/ads1115.h"
#include "hardware/i2cdevice_wrapper.h"
#include "sources/source.h"

#include <chrono>
#include <cstdint>

ADS1115Driver::ADS1115Driver(ComponentId id, DeviceRegistry& registry, EventBus& bus,
                             boost::asio::io_context& io)
    : Source(id), registry_(registry), bus_(bus), timer_(io) {
    bus_.subscribe<StartBurstSampling>([this](const StartBurstSampling& cmd) { startBurst(cmd); });
    bus_.subscribe<StopBurstSampling>([this](const StopBurstSampling&) { stopBurst(); });
}

void ADS1115Driver::update() {
    if (!std::holds_alternative<Device>(id())) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }
    auto* wrapper = registry_.get<I2CDeviceWrapper<ADS1115>>(std::get<Device>(id()));
    if (!wrapper)
        return;

    auto& adc = wrapper->device();

    for (std::size_t channel = 0; channel < 4; ++channel) {
        const auto sample = adc.getSample(channel);

        bus_.publish(
            Ads1115Event{adc.getAddress(), static_cast<std::uint8_t>(channel),
                         static_cast<std::uint16_t>(sample.value), sample.voltage,
                         static_cast<std::uint64_t>(sample.timestamp.time_since_epoch().count())});
    }
}

void ADS1115Driver::startBurst(const StartBurstSampling& cmd) {
    interval_ = std::chrono::milliseconds(1000 / cmd.frequencyHz);
    remaining_ = cmd.samples;

    scheduleBurst();
}

void ADS1115Driver::stopBurst() {
}

void ADS1115Driver::scheduleBurst() {
    if (remaining_ == 0)
        return;

    readAllChannels(); // reuse same logic

    --remaining_;

    timer_.expires_after(interval_);
    timer_.async_wait([this](auto ec) {
        if (!ec)
            scheduleBurst();
    });
}