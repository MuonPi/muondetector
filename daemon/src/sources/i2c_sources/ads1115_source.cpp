#include "sources/i2c_sources/ads1115_source.h"
#include "hardware/devices.h"
#include "hardware/i2cdevice_wrapper.h"
#include "core/logging/logger.h"


ADS1115Source::ADS1115Source(SourceId id,
                DeviceRegistry& registry,
                EventBus& bus)
    : Source(id),
        m_registry(registry),
        m_bus(bus)
{}

void ADS1115Source::update()
{
    if (!std::holds_alternative<Device>(m_id)) {
        throw std::logic_error("DeviceSource constructed with non-device ID");
    }
    auto* wrapper = m_registry.get<I2CDeviceWrapper<ADS1115>>(std::get<Device>(m_id));
    if (!wrapper) return;

    auto& adc = wrapper->device();

    for (std::size_t channel = 0; channel < 4; ++channel)
    {
        const auto sample = adc.getSample(channel);

        m_bus.publish(Ads1115Event{
            adc.getAddress(),
            static_cast<std::uint8_t>(channel),
            static_cast<std::uint16_t>(sample.value),
            sample.voltage,
            static_cast<std::uint64_t>(
                sample.timestamp.time_since_epoch().count()
            )
        });
    }
}