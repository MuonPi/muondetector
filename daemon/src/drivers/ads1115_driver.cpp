#include "drivers/ads1115_driver.h"

#include "config.h"
#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "data/events/ads1115_event.h"
#include "data/events/datastore_store_event.h"
#include "data/events/event_trigger_event.h"
#include "hardware/devices.h"
#include "hardware/i2c/ads1115.h"
#include "hardware/i2cdevice_wrapper.h"
#include "sources/source.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <utility>

ADS1115Driver::ADS1115Driver(ComponentId id, DeviceRegistry& registry, EventBus& bus,
                             boost::asio::io_context& io)
    : Source(id), registry_(registry), bus_(bus), timer_(io), traceTimer_(io) {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }

    deviceId_ = std::get<Device>(id);

    auto* device = dev();
    if (device == nullptr) {
        setAdcSamplingMode(ADC_SAMPLING_MODE::DISABLED);
        return;
    }

    bus_.subscribe<StartBurstSampling>([this](const StartBurstSampling& cmd) { startBurst(cmd); });
    bus_.subscribe<StopBurstSampling>([this](const StopBurstSampling&) { stopBurst(); });
    bus_.subscribe<AdcModeEvent>([this](const AdcModeEvent& event) {
        setAdcSamplingMode(static_cast<ADC_SAMPLING_MODE>(event.mode));
    });
    bus_.subscribe<AdcModeRequestCmd>([this](const AdcModeRequestCmd&) {
        ADC_SAMPLING_MODE mode;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            mode = adcSamplingMode_;
        }
        bus_.publish(AdcModeEvent{.mode = static_cast<std::uint8_t>(mode)});
    });
    bus_.subscribe<AdcSampleTriggerCmd>(
        [this](const AdcSampleTriggerCmd& cmd) { sampleAdcEvent(cmd.channel); });
    bus_.subscribe<EventTriggerCmd>([this](const EventTriggerCmd& cmd) {
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            eventTrigger_ = cmd.signal;
        }
        bus_.publish(DatastoreStoreEvent{EventTriggerEvent{.signal = cmd.signal}});
    });
    bus_.subscribe<GpioEvent>([this](const GpioEvent& event) {
        bool shouldSample = false;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            shouldSample = event.gpio_signal == eventTrigger_;
            if (shouldSample) {
                currentAdcSampleIndex_ = 0;
            }
        }
        if (shouldSample) {
            sampleAdcEvent(MuonPi::Config::Hardware::ADC::Channel::amplitude);
        }
    });

    device->setPga(ADS1115::PGA4V);   // set full scale range to 4 Volts
    device->setRate(ADS1115::SPS860); // set sampling rate to the maximum of 860 samples per second
    device->setAGC(false);            // turn AGC off for all channels
    if (!device->setDataReadyPinMode()) {
        logError("error: failed setting data ready pin mode (setting thresh regs)");
    }

    device->registerConversionReadyCallback(
        [this](ADS1115::Sample sample) { onSampleReady(sample); });
}

auto ADS1115Driver::dev() -> ADS1115* {
    auto* wrapper = registry_.get<I2CDeviceWrapper<ADS1115>>(std::get<Device>(id()));
    if (!wrapper) {
        logError("ADS1115 Device not found");
        return nullptr;
    }

    return &wrapper->device();
}

void ADS1115Driver::onSampleReady(ADS1115::Sample sample) {
    const auto channel = static_cast<std::uint8_t>(sample.channel);
    const float voltage = sample.voltage;
    auto* adc = dev();
    double convTime{0.};
    if (adc != nullptr) {
        convTime = adc->getLastConvTime();
    }

    auto event = ADS1115Event{
        .deviceId = static_cast<std::uint32_t>(deviceId_),
        .channel = channel,
        .rawValue = static_cast<std::uint16_t>(sample.value),
        .voltage = voltage,
        .timestamp =
            static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                           sample.timestamp.time_since_epoch())
                                           .count()),
        .convTime = convTime,
    };

    if (channel != MuonPi::Config::Hardware::ADC::Channel::amplitude) {
        bus_.publish(event);
        return;
    }

    std::vector<float> trace;
    bool publishSample = false;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);

        if (adcSamplingMode_ == ADC_SAMPLING_MODE::TRACE) {
            adcSamplesBuffer_.push_back(voltage);
            while (adcSamplesBuffer_.size() > MuonPi::Config::Hardware::ADC::buffer_size) {
                adcSamplesBuffer_.pop_front();
            }

            publishSample = currentAdcSampleIndex_ == 0;

            if (currentAdcSampleIndex_ >= 0) {
                ++currentAdcSampleIndex_;
                const auto postTriggerSamples =
                    static_cast<int>(MuonPi::Config::Hardware::ADC::buffer_size -
                                     MuonPi::Config::Hardware::ADC::pretrigger);
                if (currentAdcSampleIndex_ >= postTriggerSamples) {
                    trace.assign(adcSamplesBuffer_.begin(), adcSamplesBuffer_.end());
                    currentAdcSampleIndex_ = -1;
                }
            }
        } else if (adcSamplingMode_ == ADC_SAMPLING_MODE::PEAK) {
            publishSample = true;
            currentAdcSampleIndex_ = 0;
        }
    }

    if (publishSample) {
        bus_.publish(event);
    }
    if (!trace.empty()) {
        bus_.publish(AdcTraceEvent{.adcSampleBuffer = std::move(trace)});
    }
}

void ADS1115Driver::update() {
    auto* adc = dev();
    if (adc == nullptr) {
        return;
    }

    for (std::size_t channel = 0; channel < 4; ++channel) {
        adc->getSample(channel); // triggers function "onSampleReady"
    }
}

auto ADS1115Driver::getVoltage(std::uint8_t channel, bool& ok) -> double {
    auto* adc = dev();
    if (adc == nullptr) {
        ok = false;
        return -1.;
    }
    ok = true;
    return adc->getVoltage(channel);
}

void ADS1115Driver::startBurst(const StartBurstSampling& cmd) {
    if (cmd.frequencyHz == 0 || cmd.samples == 0) {
        stopBurst();
        return;
    }

    interval_ = std::chrono::milliseconds(std::max<std::size_t>(1, 1000 / cmd.frequencyHz));
    remaining_ = static_cast<int>(cmd.samples);
    burstSamples_.clear();
    burstSamples_.reserve(cmd.samples);

    scheduleBurst();
}

void ADS1115Driver::stopBurst() {
    remaining_ = 0;
    timer_.cancel();
}

void ADS1115Driver::scheduleBurst() {
    if (remaining_ <= 0) {
        return;
    }

    timer_.expires_after(interval_);
    timer_.async_wait([this](auto ec) {
        if (ec) {
            return;
        }

        auto* adc = dev();
        if (adc == nullptr) {
            stopBurst();
            return;
        }

        auto sample = adc->getSample(MuonPi::Config::Hardware::ADC::Channel::amplitude);
        if (sample != ADS1115::InvalidSample) {
            burstSamples_.push_back(sample.voltage);
        }

        --remaining_;
        if (remaining_ <= 0) {
            if (!burstSamples_.empty()) {
                bus_.publish(AdcTraceEvent{.adcSampleBuffer = std::move(burstSamples_)});
                burstSamples_.clear();
            }
        } else {
            scheduleBurst();
        }
    });
}

void ADS1115Driver::setAdcSamplingMode(ADC_SAMPLING_MODE mode) {
    if (mode != ADC_SAMPLING_MODE::DISABLED && mode != ADC_SAMPLING_MODE::PEAK &&
        mode != ADC_SAMPLING_MODE::TRACE) {
        logWarn("Ignoring invalid ADC sampling mode " + std::to_string(static_cast<int>(mode)));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        adcSamplingMode_ = mode;
        currentAdcSampleIndex_ = -1;
        adcSamplesBuffer_.clear();
    }

    traceTimer_.cancel();
    if (mode == ADC_SAMPLING_MODE::TRACE) {
        scheduleTraceSampling();
    }
}

void ADS1115Driver::sampleAdcEvent(std::uint8_t channel) {
    ADC_SAMPLING_MODE mode;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        mode = adcSamplingMode_;
    }

    if (mode == ADC_SAMPLING_MODE::DISABLED) {
        return;
    }

    auto* adc = dev();
    if (adc == nullptr) {
        return;
    }

    adc->getSample(channel);
}

void ADS1115Driver::sampleTrace() {
    sampleAdcEvent(MuonPi::Config::Hardware::ADC::Channel::amplitude);
}

void ADS1115Driver::scheduleTraceSampling() {
    traceTimer_.expires_after(MuonPi::Config::Hardware::trace_sampling_interval);
    traceTimer_.async_wait([this](auto ec) {
        if (ec) {
            return;
        }

        ADC_SAMPLING_MODE mode;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            mode = adcSamplingMode_;
        }
        if (mode != ADC_SAMPLING_MODE::TRACE) {
            return;
        }

        sampleTrace();
        scheduleTraceSampling();
    });
}
