#include "drivers/gpio_driver.h"

#include "core/logging/logger.h"
#include "data/commands/bias_switch_cmd.h"
#include "data/commands/gain_switch_cmd.h"
#include "data/commands/gpio_signal_set_cmd.h"
#include "data/commands/polarity_switch_cmd.h"
#include "data/commands/preamp_switch_cmd.h"
#include "data/events/bias_switch_event.h"
#include "data/events/datastore_store_event.h"
#include "data/events/gain_switch_event.h"
#include "data/events/gpio_event.h"
#include "data/events/gpio_rate_event.h"
#include "data/events/polarity_switch_event.h"
#include "data/events/preamp_switch_event.h"
#include "data/events/status_led_event.h"
#include "utility/gpio_ratebuffer.h"

#include <gpiod.h>
#include <iostream>
#include <optional>
#include <poll.h>
#include <set>
#include <sstream>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace std::chrono;

GpioDriver::GpioDriver(ComponentId id, const std::string& chipPath, EventBus& bus)
    : Component(id), bus_(bus) {
    if (!std::holds_alternative<OtherComponent>(id)) {
        throw std::logic_error("NonDeviceSource constructed with device ID");
    }

    chip = gpiod_chip_open(chipPath.c_str());
    if (!chip) {
        throw std::runtime_error("Failed to open gpiochip");
    }

    control_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (control_fd < 0) {
        throw std::runtime_error("Failed to create control_fd");
    }

    // For all set commands we can also have some attached events
    // For example: BiasSwitchEvent should be triggered if the bias switch state changes
    // DatastoreStoreEvent will make sure that this state is stored in the datastore before fanned
    // out to Tcp Connection etc.
    bus_.subscribe<PolaritySwitchCmd>([this](const PolaritySwitchCmd& cmd) {
        bool ok{false};
        ok = writeSignal(IN_POL1, cmd.pol1);
        if (!ok) {
            return;
        }
        ok = writeSignal(IN_POL2, cmd.pol2);
        if (!ok) {
            return;
        }
        bus_.publish(DatastoreStoreEvent{PolaritySwitchEvent{.pol1 = cmd.pol1, .pol2 = cmd.pol2}});
    });
    bus_.subscribe<BiasSwitchCmd>([this](const BiasSwitchCmd& cmd) {
        bus_.publish(
            GpioSignalSetCmd{.sig = UBIAS_EN, .on = biasInverted ? !cmd.state : cmd.state});
    });
    bus_.subscribe<PreampSwitchCmd>([this](const PreampSwitchCmd& cmd) {
        GPIO_SIGNAL signal;
        if (cmd.channel == 0) {
            signal = PREAMP_1;
        } else if (cmd.channel == 1) {
            signal = PREAMP_2;
        } else {
            logError("Tried to write to not existing preamp switch " + std::to_string(cmd.channel));
            return;
        }
        bus_.publish(GpioSignalSetCmd{.sig = signal, .on = cmd.state});
    });
    bus_.subscribe<GainSwitchCmd>([this](const GainSwitchCmd& cmd) {
        bus_.publish(GpioSignalSetCmd{.sig = GAIN_HL, .on = cmd.state});
    });
    bus_.subscribe<GpioSignalSetCmd>([this](const GpioSignalSetCmd& cmd) {
        auto ok = writeSignal(cmd.sig, cmd.on);
        std::stringstream sstr;
        sstr << "Write GPIO signal " << std::to_string(static_cast<std::uint16_t>(cmd.sig));
        if (!ok) {
            sstr << "...failed!";
            logError(sstr.str());
            return;
        }
        sstr << "...success!";
        logDebug(sstr.str());
        constexpr int milliseconds{20};
        switch (cmd.sig) {
            case UBIAS_EN:
                bus_.publish(DatastoreStoreEvent{
                    BiasSwitchEvent{.biasOn = biasInverted ? !cmd.on : cmd.on}});
                break;
            case PREAMP_1:
                bus_.publish(DatastoreStoreEvent{PreampSwitchEvent{.channel = 0, .state = cmd.on}});
                break;
            case PREAMP_2:
                bus_.publish(DatastoreStoreEvent{PreampSwitchEvent{.channel = 1, .state = cmd.on}});
                break;
            case IN_POL1:
            case IN_POL2:
                logError("Unexpected gpio signal " +
                         std::to_string(static_cast<unsigned>(cmd.sig)) +
                         ". Please use PolaritySwitchCmd for setting polarity switch setting, not "
                         "GpioSignalSetCmd directly. " +
                         "Current implementation prevents forwarding state changes to event bus.");
                break;
            case GAIN_HL:
                bus_.publish(DatastoreStoreEvent{GainSwitchEvent{.state = cmd.on}});
                break;
            case STATUS1:
                bus_.publish(DatastoreStoreEvent{StatusLedEvent{
                    .sig = cmd.sig, .durationMillisec = milliseconds, .on = cmd.on}});
                break;
            case STATUS2:
                bus_.publish(DatastoreStoreEvent{StatusLedEvent{
                    .sig = cmd.sig, .durationMillisec = milliseconds, .on = cmd.on}});
                break;
            case STATUS3:
                bus_.publish(DatastoreStoreEvent{StatusLedEvent{
                    .sig = cmd.sig, .durationMillisec = milliseconds, .on = cmd.on}});
                break;
            default:
                break;
        }
    });
}

GpioDriver::~GpioDriver() {
    stop();

    if (control_fd >= 0) {
        close(control_fd);
    }

    // reset states
    for (const auto& [sig, desc] : GPIO_SIGNAL_MAP) {

        if (sig == UNDEFINED_SIGNAL)
            continue;

        auto it = pinmap_.find(sig);
        if (it == pinmap_.end())
            continue;

        if (desc.direction == DIR_OUT) {
            bool value = false;
            if (sig == UBIAS_EN) {
                value = biasInverted ? true : false;
            }
            writeGpio(it->second, value);
        }
    }

    for (auto req : inputRequests) {
        gpiod_line_request_release(req);
    }
    for (auto req : outputRequests) {
        gpiod_line_request_release(req);
    }

    if (chip)
        gpiod_chip_close(chip);
}

// WTF is this
// void Daemon::onRateBufferReminder()
// {
//     qreal secsSinceStart = 0.001 * (qreal)msecdiff(lastRateInterval, startOfProgram);
//     qreal xorRate = getRateFromCounts(XOR_RATE);
//     qreal andRate = getRateFromCounts(AND_RATE);
//     QPointF xorPoint(secsSinceStart, xorRate);
//     QPointF andPoint(secsSinceStart, andRate);
//     xorRatePoints.append(xorPoint);
//     andRatePoints.append(andPoint);
//     sendGpioRatesAverage(XOR_RATE, xorRate);
//     sendGpioRatesAverage(AND_RATE, andRate);
//     emit logParameter(LogParameter("rateXOR", QString::number(xorRate) + " Hz",
//     LogParameter::LOG_AVERAGE)); emit logParameter(LogParameter("rateAND",
//     QString::number(andRate) + " Hz", LogParameter::LOG_AVERAGE)); while
//     ((quint32)xorRatePoints.size() > rateMaxShowInterval / rateBufferInterval) {
//         xorRatePoints.pop_front();
//     }
//     while ((quint32)andRatePoints.size() > rateMaxShowInterval / rateBufferInterval) {
//         andRatePoints.pop_front();
//     }
// }
void GpioDriver::sendGpioRatesAverage() {
}

void GpioDriver::init(const MuonPi::Version::Version& hardwareVersion) {

    biasInverted = (MuonPi::Version::hardware.major == 1) ? false : true;
    pinmap_ = GPIO_PINMAP_VERSIONS[hardwareVersion.major];

    for (auto& [sig, gpio] : pinmap_) {
        r_pinmap_[gpio] = sig;
    }

    std::vector<unsigned int> inputs;
    std::vector<unsigned int> outputs;

    std::stringstream sstr;
    sstr << "GPIO pin mapping:" << std::endl;
    for (const auto& [sig, desc] : GPIO_SIGNAL_MAP) {

        if (sig == UNDEFINED_SIGNAL)
            continue;

        auto it = pinmap_.find(sig);
        if (it == pinmap_.end())
            continue;

        sstr << desc.name << " \t: " << std::dec << it->second;
        switch (desc.direction) {
            case DIR_IN:
                sstr << " (in)";
                break;
            case DIR_OUT:
                sstr << " (out)";
                break;
            case DIR_IO:
                sstr << " (i/o)";
                break;
            default:
                sstr << " (undef)";
        }

        if (desc.direction == DIR_IN)
            inputs.push_back(it->second);
        else if (desc.direction == DIR_OUT)
            outputs.push_back(it->second);
    }

    configureLines(inputs, {.dir = SIGNAL_DIRECTION::DIR_IN, .edge = GPIOD_LINE_EDGE_BOTH});

    configureLines(outputs, {.dir = SIGNAL_DIRECTION::DIR_OUT, .initialValue = false});

    // set up rate buffers for all GPIO interrupts
    gpioRatebuffers.emplace(EVT_AND, std::make_shared<EventRateBuffer>(bus_, EventEdge::Rising));
    // gpioRatebuffers.emplace(EVT_XOR, std::make_shared<EventRateBuffer>(bus_, EventEdge::Rising));
    auto vetoRateBuffer =
        std::make_shared<CoincidenceEventBuffer>(bus_, EventEdge::Rising, TIME_MEAS_OUT, true);
    auto rateBufferTime = std::chrono::seconds(10);
    vetoRateBuffer->setBufferTime(rateBufferTime);
    gpioRatebuffers.emplace(EVT_XOR, vetoRateBuffer);

    start();
}

void GpioDriver::processEvent(GpioEvent&& event) {
    // could just publish here
    // bus_.publish<GpioEvent>(event);

    // or do some rate buffering

    auto it = gpioRatebuffers.find(event.gpio_signal);
    if (it != gpioRatebuffers.end()) {
        it->second->handle(event);
        return;
    }
    // if not have rate buffer for event, just publish it
    bus_.publish(event);
}

void GpioDriver::start() {
    if (running.exchange(true))
        return;

    worker = std::thread([this]() { eventLoop(); });
}

void GpioDriver::stop() {
    if (!running.exchange(false))
        return;

    // wake poll immediately
    if (control_fd >= 0) {
        uint64_t one = 1;
        write(control_fd, &one, sizeof(one));
    }

    if (worker.joinable())
        worker.join();
}

void GpioDriver::eventLoop() {

    gpiod_edge_event_buffer* buffer = gpiod_edge_event_buffer_new(128);

    std::vector<struct pollfd> fds;

    // Build fd list once
    for (auto req : inputRequests) {
        int fd = gpiod_line_request_get_fd(req);
        fds.push_back({fd, POLLIN, 0});
        fdMap[fd] = req;
    }
    logDebug("Monitoring " + std::to_string(fds.size()) + " GPIO request fds");

    // ADD control fd
    fds.push_back({control_fd, POLLIN, 0});

    while (running.load()) {
        for (auto& p : fds) {
            p.revents = 0;
        }

        int ret = poll(fds.data(), fds.size(), -1); // block

        if (!running.load())
            break;

        if (ret <= 0)
            continue;

        for (auto& p : fds) {

            // CONTROL FD CASE
            if (p.fd == control_fd && (p.revents & POLLIN)) {
                uint64_t dummy;
                read(control_fd, &dummy, sizeof(dummy)); // clear event
                continue;
            }

            if (!(p.revents & POLLIN))
                continue;

            auto req = fdMap.at(p.fd);

            int n = gpiod_line_request_read_edge_events(req, buffer, 128);

            for (int i = 0; i < n; i++) {

                auto* ev = gpiod_edge_event_buffer_get_event(buffer, i);

                unsigned int offset = gpiod_edge_event_get_line_offset(ev);

                GpioEvent out;
                out.gpio_pin = offset;
                out.gpio_signal = r_pinmap_.at(offset);
                out.timestamp = std::chrono::steady_clock::time_point(
                    std::chrono::nanoseconds(gpiod_edge_event_get_timestamp_ns(ev)));

                out.edge = gpiod_edge_event_get_event_type(ev) == GPIOD_EDGE_EVENT_RISING_EDGE
                               ? EventEdge::Rising
                               : EventEdge::Falling;

                processEvent(std::move(out));
                logDebug("GpioEvent: " + std::to_string(out.gpio_pin) +
                         " edge: " + (out.edge == EventEdge::Rising ? "rising" : "falling"));
            }
        }
    }

    gpiod_edge_event_buffer_free(buffer);
    logDebug("Finished GPIO event loop");
}

auto GpioDriver::configureLines(const std::vector<unsigned int>& gpios,
                                const LineConfig& cfg) -> bool {

    if (gpios.empty())
        return true;

    auto* settings = gpiod_line_settings_new();

    if (cfg.dir == SIGNAL_DIRECTION::DIR_IN) {

        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);

        gpiod_line_settings_set_edge_detection(settings, cfg.edge);

        // Highly recommended on Pi:
        gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    } else {

        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);

        gpiod_line_settings_set_output_value(
            settings, cfg.initialValue ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
    }

    auto* line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, gpios.data(), gpios.size(), settings);

    auto* req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "muonpi");

    auto* req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    if (!req) {
        std::cerr << "Failed to configure GPIO group\n";
        return false;
    }

    // Store request once
    if (cfg.dir == SIGNAL_DIRECTION::DIR_IN) {
        inputRequests.insert(req);
    } else {
        outputRequests.insert(req);
    }

    // Map each GPIO to (request + offset)
    for (auto gpio : gpios) {
        gpioMap[gpio] = {req, gpio};
    }

    return true;
}

auto GpioDriver::writeSignal(GPIO_SIGNAL sig, bool value) -> bool {
    auto ok = writeGpio(pinmap_.at(sig), value);
    if (ok) {
        bus_.publish(GpioEvent{.gpio_signal = sig,
                               .gpio_pin = pinmap_.at(sig),
                               .timestamp = EventTime::clock().now(),
                               .edge = value ? EventEdge::Rising : EventEdge::Falling});
    } else {
        logError("Failed to write signal " + std::to_string(static_cast<unsigned>(sig)) + " gpio " +
                 std::to_string(pinmap_.at(sig)));
    }
    return ok;
}

auto GpioDriver::writeGpio(unsigned int gpio, bool value) -> bool {

    auto it = gpioMap.find(gpio);
    if (it == gpioMap.end())
        return false;

    return gpiod_line_request_set_value(it->second.req, it->second.offset,
                                        value ? GPIOD_LINE_VALUE_ACTIVE
                                              : GPIOD_LINE_VALUE_INACTIVE) == 0;
}