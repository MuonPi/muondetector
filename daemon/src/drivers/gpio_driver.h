#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "config.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "data/events/gpio_event.h"
#include "gpio_pin_definitions.h"
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

class EventRateBuffer;
class GpioDriver : public Component {
  public:
    struct GpioLine {
        gpiod_line_request* req;
        unsigned int offset;
    };
    GpioDriver(ComponentId id, const std::string& chipPath, EventBus& bus);
    ~GpioDriver();

    void init(const MuonPi::Version::Version& hardwareVersion);
    auto writeSignal(GPIO_SIGNAL sig, bool value) -> bool;

  private:
    auto configureLines(const std::vector<unsigned int>& gpios, const LineConfig& cfg) -> bool;
    void eventLoop();
    void processEvent(GpioEvent&& event);
    void start();
    void stop();
    auto writeGpio(unsigned int gpio, bool value) -> bool;
    gpiod_chip* chip = nullptr;
    EventBus& bus_;

    std::map<GPIO_SIGNAL, unsigned int> pinmap_;
    std::map<unsigned int, GPIO_SIGNAL> r_pinmap_;

    std::set<gpiod_line_request*> inputRequests;
    std::set<gpiod_line_request*> outputRequests;
    std::map<unsigned int, GpioLine> gpioMap;
    std::map<int, gpiod_line_request*> fdMap;

    std::unordered_map<GPIO_SIGNAL, std::shared_ptr<EventRateBuffer>> gpioRatebuffers;

    std::thread worker;
    int control_fd{-1};
    std::atomic<bool> running{false};
};

#endif // GPIO_DRIVER_H
