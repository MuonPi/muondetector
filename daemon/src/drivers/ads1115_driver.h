#ifndef ADS1115_DRIVER_H
#define ADS1115_DRIVER_H

#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "data/commands/adc_mode_request_cmd.h"
#include "data/commands/adc_sample_trigger_cmd.h"
#include "data/commands/burst_sampling_cmd.h"
#include "data/commands/event_trigger_cmd.h"
#include "data/events/adc_mode_event.h"
#include "data/events/adc_trace_event.h"
#include "data/events/ads1115_event.h"
#include "data/events/gpio_event.h"
#include "data/muondetector_structs.h"
#include "hardware/i2c/ads1115.h"
#include "hardware/i2cdevice_wrapper.h"
#include "sources/source.h"

#include <boost/asio.hpp>
#include <deque>
#include <mutex>
#include <vector>

class ADS1115Driver : public Source {
  public:
    ADS1115Driver(ComponentId id, DeviceRegistry& registry, EventBus& bus,
                  boost::asio::io_context& io);

    void update() override;
    auto getVoltage(std::uint8_t channel, bool& ok) -> double;

  private:
    void startBurst(const StartBurstSampling& cmd);
    void stopBurst();
    void setAdcSamplingMode(ADC_SAMPLING_MODE mode);
    void sampleAdcEvent(std::uint8_t channel);
    void sampleTrace();

  private:
    auto dev() -> ADS1115*;
    void onSampleReady(ADS1115::Sample sample);
    void scheduleBurst();
    void scheduleTraceSampling();
    DeviceRegistry& registry_;
    EventBus& bus_;
    boost::asio::steady_timer timer_;
    boost::asio::steady_timer traceTimer_;

    Device deviceId_;

    std::chrono::milliseconds interval_;
    int remaining_ = 0;
    std::vector<float> burstSamples_;
    std::deque<float> adcSamplesBuffer_;
    std::mutex stateMutex_;
    ADC_SAMPLING_MODE adcSamplingMode_{ADC_SAMPLING_MODE::PEAK};
    GPIO_SIGNAL eventTrigger_{EVT_XOR};
    int currentAdcSampleIndex_{-1};
};

#endif // ADS_1115_SOURCE_H
