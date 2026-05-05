#include "drivers/gpio_driver.h"

#include "config.h"
#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "gpio_pin_definitions.h"
#include "utility/gpio_mapping.h"

#include <cmath>
#include <config.h>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/time.h>
#include <time.h>

using namespace std::chrono;

GpioDriver::GpioDriver(ComponentId id, const std::string& chipPath, EventBus& bus)
    : Component(id), chipPath(std::move(chipPath)), bus_(bus) {
    chip = gpiod_chip_open(chipPath.c_str());
    if (!chip) {
        throw std::runtime_error("Failed to open gpiochip");
    }
}

void GpioDriver::init(const MuonPi::Version::Version& hardwareVersion) {
    pinmap_ = GPIO_PINMAP_VERSIONS[hardwareVersion.major];

    for (auto& [key, value] : pinmap_) {
        r_pinmap_.emplace(value, key);
    }

    // // set up the pin definitions (hw version specific)
    // GPIO_PINMAP = GPIO_PINMAP_VERSIONS[MuonPi::Version::hardware.major];

    std::stringstream sstr;
    // print out the current gpio pin mapping
    // (function, gpio-pin, direction)
    sstr << "GPIO pin mapping:" << std::endl;

    for (auto signalIt = pinmap_.begin(); signalIt != pinmap_.end(); signalIt++) {
        const GPIO_SIGNAL signalId = signalIt->first;
        sstr << GPIO_SIGNAL_MAP.at(signalId).name << " \t: " << std::dec << signalIt->second;
        switch (GPIO_SIGNAL_MAP.at(signalId).direction) {
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
        sstr << std::endl;
    }
    logDebug(sstr.str());

    std::vector<unsigned int> inputs;
    std::vector<unsigned int> outputs;

    for (const auto& [sig, desc] : GPIO_SIGNAL_MAP) {

        if (sig == UNDEFINED_SIGNAL)
            continue;

        auto pinIt = pinmap_.find(sig);
        if (pinIt == pinmap_.end())
            continue;

        switch (desc.direction) {
            case DIR_IN:
                inputs.push_back(pinIt->second);
                break;

            case DIR_OUT:
                outputs.push_back(pinIt->second);
                break;

            default:
                break;
        }
    }
    configureLines(inputs, {.dir = SIGNAL_DIRECTION::DIR_IN, .edgeBoth = false});

    configureLines(outputs, {.dir = SIGNAL_DIRECTION::DIR_OUT, .initialValue = false});

    start();
}

GpioDriver::~GpioDriver() {
    stop();

    for (auto& [gpio, req] : requests) {
        gpiod_line_request_release(req);
    }

    if (chip)
        gpiod_chip_close(chip);
}

void GpioDriver::start() {
    if (running.exchange(true))
        return;

    worker = std::thread([this]() { eventLoop(); });
}

void GpioDriver::stop() {
    if (!running.exchange(false))
        return;

    if (worker.joinable())
        worker.join();
}

void GpioDriver::eventLoop() {
    gpiod_edge_event_buffer* buffer = gpiod_edge_event_buffer_new(64);

    while (running.load()) {

        for (auto& [gpio, req] : requests) {

            int ret = gpiod_line_request_wait_edge_events(req,
                                                          100000000 // 100 ms in ns
            );

            if (ret <= 0)
                continue;

            int n = gpiod_line_request_read_edge_events(req, buffer, 64);

            for (int i = 0; i < n; i++) {

                auto* ev = gpiod_edge_event_buffer_get_event(buffer, i);

                GpioEvent out;
                out.gpio_pin = gpiod_edge_event_get_line_offset(ev);
                out.gpio_signal = r_pinmap_.at(out.gpio_pin);
                out.timestamp = std::chrono::nanoseconds(gpiod_edge_event_get_timestamp_ns(ev));

                out.edge = gpiod_edge_event_get_event_type(ev) == GPIOD_EDGE_EVENT_RISING_EDGE
                               ? EventEdge::Rising
                               : EventEdge::Falling;

                bus_.publish<GpioEvent>(std::move(out));
            }
        }
    }

    gpiod_edge_event_buffer_free(buffer);
}

bool GpioDriver::configureLines(const std::vector<unsigned int>& gpios, const LineConfig& cfg) {
    gpiod_line_settings* settings = gpiod_line_settings_new();

    if (cfg.dir == SIGNAL_DIRECTION::DIR_IN) {
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);

        if (cfg.edgeBoth) {
            gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
        }
    } else {
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);

        gpiod_line_settings_set_output_value(
            settings, cfg.initialValue ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
    }

    gpiod_line_config* line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, gpios.data(), gpios.size(), settings);

    gpiod_request_config* req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "muonpi");

    gpiod_line_request* req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    if (!req) {
        std::cerr << "Failed to configure GPIO group\n";
        return false;
    }

    // IMPORTANT: replace old request if needed
    for (auto gpio : gpios) {
        auto it = requests.find(gpio);
        if (it != requests.end()) {
            gpiod_line_request_release(it->second);
            requests.erase(it);
        }
        requests[gpio] = req;
    }

    return true;
}

bool GpioDriver::write(unsigned int gpio, bool value) {
    auto it = requests.find(gpio);
    return gpiod_line_request_set_value(
               it->second, gpio, value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE) == 0;
}
