#ifndef ADS1115_DRIVER_H
#define ADS1115_DRIVER_H

#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "data/commands/burst_sampling_cmd.h"
#include "data/events/ads1115_event.h"
#include "data/muondetector_structs.h"
#include "hardware/i2c/ads1115.h"
#include "hardware/i2cdevice_wrapper.h"
#include "sources/source.h"

#include <boost/asio.hpp>

class ADS1115Driver : public Source {
  public:
    ADS1115Driver(ComponentId id, DeviceRegistry& registry, EventBus& bus,
                  boost::asio::io_context& io);

    void update() override;

  private:
    void startBurst(const StartBurstSampling& cmd);
    void stopBurst();

  private:
    auto dev() -> ADS1115*;
    void onSampleReady(ADS1115::Sample sample);
    void scheduleBurst();
    DeviceRegistry& registry_;
    EventBus& bus_;
    boost::asio::steady_timer timer_;

    Device deviceId_;

    std::chrono::milliseconds interval_;
    int remaining_ = 0;
    ADC_SAMPLING_MODE adcSamplingMode{ADC_SAMPLING_MODE::PEAK};
};

#endif // ADS_1115_SOURCE_H