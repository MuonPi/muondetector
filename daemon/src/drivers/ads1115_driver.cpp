
#include "drivers/ads1115_driver.h"

#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "data/events/ads1115_event.h"
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
    const uint8_t channel = sample.channel;
    float voltage = sample.voltage;
    if (channel != 0) {
        bus_.publish(Ads1115Event{
            .deviceId = static_cast<std::uint32_t>(deviceId_),
            .channel = channel,
            .rawValue = static_cast<std::uint16_t>(sample.value),
            .voltage = voltage,
            .timestamp =
                static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                               sample.timestamp.time_since_epoch())
                                               .count()),
            // .samplingMode = ADC_SAMPLING_MODE::PEAK
        });
    }
    //  else {
    //     if (adcSamplingMode == ADC_SAMPLING_MODE::TRACE) {
    //         adcSamplesBuffer.push_back(voltage);
    //         if (adcSamplesBuffer.size() > Config::Hardware::ADC::buffer_size) {
    //             adcSamplesBuffer.pop_front();
    //         }
    //         if (currentAdcSampleIndex == 0) {
    //             TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_SAMPLE);
    //             *(tcpMessage.dStream) << (quint8)channel << voltage;
    //             emit sendTcpMessage(tcpMessage);
    //             m_histo_map["pulseHeight"]->fill(voltage);
    //         }
    //         if (currentAdcSampleIndex >= 0) {
    //             currentAdcSampleIndex++;
    //             if (currentAdcSampleIndex >= static_cast<int>(Config::Hardware::ADC::buffer_size
    //             - Config::Hardware::ADC::pretrigger)) {
    //                 TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_TRACE);
    //                 *(tcpMessage.dStream) << (quint16)adcSamplesBuffer.size();
    //                 for (auto adc_sample : adcSamplesBuffer) {
    //                     *(tcpMessage.dStream) << adc_sample;
    //                 }
    //                 emit sendTcpMessage(tcpMessage);
    //                 currentAdcSampleIndex = -1;
    //             }
    //         }
    //     } else if (adcSamplingMode == ADC_SAMPLING_MODE::PEAK) {
    //         TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_SAMPLE);
    //         *(tcpMessage.dStream) << (quint8)channel << voltage;
    //         emit sendTcpMessage(tcpMessage);
    //         m_histo_map["pulseHeight"]->fill(voltage);
    //         currentAdcSampleIndex = 0;
    //     }
    // }
    // if (adc_p) {
    //     emit logParameter(LogParameter("adcSamplingTime",
    //     QString::number(adc_p->getLastConvTime()) + " ms", LogParameter::LOG_AVERAGE));
    //     m_histo_map["adcSampleTime"]->fill(adc_p->getLastConvTime());
    // }
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