#include "drivers/gpio_driver.h"

#include "core/logging/logger.h"
#include "events/gpio_event.h"

#include <gpiod.h>
#include <iostream>
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
}

GpioDriver::~GpioDriver() {
    stop();

    if (control_fd >= 0) {
        close(control_fd);
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

void GpioDriver::init(const MuonPi::Version::Version& hardwareVersion) {

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

    start();
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
                out.timestamp = std::chrono::nanoseconds(gpiod_edge_event_get_timestamp_ns(ev));

                out.edge = gpiod_edge_event_get_event_type(ev) == GPIOD_EDGE_EVENT_RISING_EDGE
                               ? EventEdge::Rising
                               : EventEdge::Falling;

                bus_.publish<GpioEvent>(std::move(out));
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
    return writeGpio(pinmap_.at(sig), value);
}

auto GpioDriver::writeGpio(unsigned int gpio, bool value) -> bool {

    auto it = gpioMap.find(gpio);
    if (it == gpioMap.end())
        return false;

    return gpiod_line_request_set_value(it->second.req, it->second.offset,
                                        value ? GPIOD_LINE_VALUE_ACTIVE
                                              : GPIOD_LINE_VALUE_INACTIVE) == 0;
}