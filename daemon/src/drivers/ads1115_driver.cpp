
#include "drivers/ads1115_driver.h"

#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "data/events/ads1115_event.h"
#include "data/events/datastore_store_event.h"
#include "hardware/devices.h"
#include "hardware/i2c/ads1115.h"
#include "hardware/i2cdevice_wrapper.h"
#include "sources/source.h"

#include <chrono>
#include <cstdint>

ADS1115Driver::ADS1115Driver(ComponentId id, DeviceRegistry& registry, EventBus& bus,
                             boost::asio::io_context& io)
    : Source(id), registry_(registry), bus_(bus), timer_(io) {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }
    auto* device = dev();
    if (device == nullptr) {
        return;
    }

    bus_.subscribe<StartBurstSampling>([this](const StartBurstSampling& cmd) { startBurst(cmd); });
    bus_.subscribe<StopBurstSampling>([this](const StopBurstSampling&) { stopBurst(); });

    device->setPga(ADS1115::PGA4V);   // set full scale range to 4 Volts
    device->setRate(ADS1115::SPS860); // set sampling rate to the maximum of 860 samples per second
    device->setAGC(false);            // turn AGC off for all channels
    if (!device->setDataReadyPinMode()) {
        logWarn("error: failed setting data ready pin mode (setting thresh regs)");
    }

    device->registerConversionReadyCallback(
        [this](ADS1115::Sample sample) { onSampleReady(sample); });
}

auto ADS1115Driver::dev() -> ADS1115* {
    auto* wrapper = registry_.get<I2CDeviceWrapper<ADS1115>>(std::get<Device>(id()));
    if (!wrapper) {
        logWarn("ADS1115 Device not found");
        return nullptr;
    }

    return &wrapper->device();
}

void ADS1115Driver::onSampleReady(ADS1115::Sample sample) {
}

void ADS1115Driver::update() {
    auto* adc = dev();
    if (adc == nullptr) {
        return;
    }

    DatastoreStoreEvent<std::array<Ads1115Event, 4>> storeEvent;

    for (std::size_t channel = 0; channel < 4; ++channel) {
        const auto sample = adc->getSample(channel);

        storeEvent.data.at(channel) =
            Ads1115Event{adc->getAddress(), static_cast<std::uint8_t>(channel),
                         static_cast<std::uint16_t>(sample.value), sample.voltage,
                         static_cast<std::uint64_t>(sample.timestamp.time_since_epoch().count())};
    }
    bus_.publish(storeEvent); // Will be unpacked later and sent to frontend
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

    // readAllChannels(); // reuse same logic

    --remaining_;

    timer_.expires_after(interval_);
    timer_.async_wait([this](auto ec) {
        if (!ec)
            scheduleBurst();
    });
}