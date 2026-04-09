#include "sources/i2c_sources/ads1115_source.h"

ADS1115Source::ADS1115Source(uint32_t id,
                DeviceRegistry& registry,
                EventBus& bus)
    : m_id(id),
        m_registry(registry),
        m_bus(bus)
{}

void ADS1115Source::update()
{
    auto* adc = m_registry.get<ADS1115>(m_id);
    if (!adc) return;

    for (std::size_t channel{0}; channel < 4; channel++) {
        ADS1115::Sample sample = adc->getSample(channel);
        Ad1115SampleEvent event{
            adc->getAddress(),
            static_cast<std::uint8_t>(channel),
            static_cast<std::uint16_t>(sample.value),
            sample.voltage,
            static_cast<std::uint64_t>(sample.timestamp.time_since_epoch().count())
        };

        m_bus.publish(event);
    }
}
