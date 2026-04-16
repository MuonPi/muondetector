#ifndef ADS1115_DRIVER_H
#define ADS1115_DRIVER_H

#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "data/commands/burst_sampling_cmd.h"
#include "data/events/ad1115_event.h"
#include "hardware/i2cdevice_wrapper.h"
#include "hardware/i2c/ads1115.h"
#include "sources/source.h"
#include <boost/asio.hpp>

class ADS1115Driver : public Source
{
  public:
    ADS1115Driver(ComponentId id, DeviceRegistry &registry, EventBus &bus, boost::asio::io_context &io);

    void update() override;

  private:
    void readAllChannels()
    {
        auto *wrapper = registry_.get<I2CDeviceWrapper<ADS1115>>(deviceId_);
        if (!wrapper)
            return;

        auto &adc = wrapper->device();

        for (std::size_t channel = 0; channel < 4; ++channel)
        {
            const auto sample = adc.getSample(channel);

            bus_.publish(Ads1115Event{adc.getAddress(), static_cast<uint8_t>(channel),
                                      static_cast<uint16_t>(sample.value), sample.voltage,
                                      static_cast<uint64_t>(sample.timestamp.time_since_epoch().count())});
        }
    }

    void startBurst(const StartBurstSampling &cmd);
    void stopBurst();

  private:
    void scheduleBurst();
    DeviceRegistry &registry_;
    EventBus &bus_;
    boost::asio::steady_timer timer_;

    Device deviceId_;

    std::chrono::milliseconds interval_;
    int remaining_ = 0;
};

#endif // ADS_1115_SOURCE_H