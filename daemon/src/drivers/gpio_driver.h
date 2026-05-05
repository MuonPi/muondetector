#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "config.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "utility/gpio_mapping.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <gpiod.h>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class GpioDriver : public Component {
  public:
    struct GpioLine {
        gpiod_line_request* req;
        unsigned int offset;
    };
    GpioDriver(ComponentId id, const std::string& chipPath, EventBus& bus);
    ~GpioDriver();

    void init(const MuonPi::Version::Version& hardwareVersion);
    bool writeGpio(unsigned int gpio, bool value);

  private:
    bool configureLines(const std::vector<unsigned int>& gpios, const LineConfig& cfg);
    void eventLoop();
    void start();
    void stop();
    gpiod_chip* chip = nullptr;
    EventBus& bus_;

    std::map<GPIO_SIGNAL, unsigned int> pinmap_;
    std::map<unsigned int, GPIO_SIGNAL> r_pinmap_;

    std::set<gpiod_line_request*> inputRequests;
    std::set<gpiod_line_request*> outputRequests;
    std::map<unsigned int, GpioLine> gpioMap;
    std::map<int, gpiod_line_request*> fdMap;

    std::thread worker;
    int control_fd{-1};
    std::atomic<bool> running{false};
};

#endif // GPIO_DRIVER_H
