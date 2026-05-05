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
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class GpioDriver : public Component {
  public:
    GpioDriver(ComponentId id, const std::string& chip, EventBus& bus);
    ~GpioDriver();

    void init(const MuonPi::Version::Version& hardwareVersion);
    bool write(unsigned int gpio, bool value);

  private:
    bool configureLines(const std::vector<unsigned int>& gpios, const LineConfig& cfg);
    void eventLoop();
    void start();
    void stop();
    std::string chipPath;
    gpiod_chip* chip = nullptr;
    EventBus& bus_;

    std::map<GPIO_SIGNAL, unsigned int> pinmap_;
    std::map<unsigned int, GPIO_SIGNAL> r_pinmap_;

    std::unordered_map<unsigned int, gpiod_line_request*> requests;

    std::thread worker;
    std::atomic<bool> running{false};
};

#endif // GPIO_DRIVER_H
