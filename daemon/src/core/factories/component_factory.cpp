#include "component_factory.h"

#include "core/context.h"
// #include "hardware/devices.h"
#include "core/component_config.h"
// #include "core/logging/logger.h"
#include "drivers/adafruit_ssd1306_driver.h"
#include "drivers/ads1115_driver.h"
#include "drivers/eeprom24aa02_driver.h"
#include "drivers/gpio_driver.h"
#include "drivers/mcp4728_driver.h"
#include "drivers/pca9536_driver.h"
#include "hardware/ublox/serialublox.h"
#include "sources/tcp_command_decoder.h"
#include "sources/tcp_source.h"
#include "sources/temp_source.h"

#include <memory>
#include <type_traits>
#include <variant>

auto ComponentFactory::createComponent(const ComponentConfig& config,
                                       Context& ctx) -> std::shared_ptr<Component> {
    return std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Device>) {
                auto it = deviceComponentCreator.find(arg);
                if (it == deviceComponentCreator.end()) {
                    throw std::runtime_error("No factory method found for component " +
                                             std::to_string(static_cast<unsigned>(arg)));
                }
                return it->second(ctx);
            } else if constexpr (std::is_same_v<T, OtherComponent>) {
                auto it = componentCreator.find(arg);
                if (it == componentCreator.end()) {
                    throw std::runtime_error("No factory method found for component " +
                                             std::to_string(static_cast<unsigned>(arg)));
                }
                return it->second(ctx);
            } else {
                static_assert(false, "non-exhaustive visitor!");
            }
        },
        config.id);
}

const std::unordered_map<ComponentId, DeviceComponentCreator>
    ComponentFactory::deviceComponentCreator = {
        {Device::ADS1115_0,
         [](Context& ctx) {
             return std::make_shared<ADS1115Driver>(Device::ADS1115_0, *ctx.registry, *ctx.bus,
                                                    *ctx.io);
         }},
        {Device::MCP4728_0,
         [](Context& ctx) {
             return std::make_shared<MCP4728Driver>(Device::MCP4728_0, *ctx.config, *ctx.registry,
                                                    *ctx.bus);
         }},
        {Device::MIC184_0,
         [](Context& ctx) { return std::make_shared<TempSource>(Device::MIC184_0, *ctx.bus); }},
        {Device::LM75_0,
         [](Context& ctx) { return std::make_shared<TempSource>(Device::LM75_0, *ctx.bus); }},
        {Device::PCA9536_0,
         [](Context& ctx) {
             return std::make_shared<PCA9536Driver>(Device::PCA9536_0, *ctx.config, *ctx.registry,
                                                    *ctx.bus);
         }},
        {Device::EEPROM24AA02_0,
         [](Context& ctx) {
             return std::make_shared<EEPROM24AA02Driver>(Device::EEPROM24AA02_0, *ctx.registry,
                                                         *ctx.bus);
         }},
        {Device::ADAFRUIT_SSD1306_0, [](Context& ctx) {
             return std::make_shared<Adafruit_SSD1306Driver>(Device::ADAFRUIT_SSD1306_0,
                                                             *ctx.config, *ctx.registry, *ctx.bus);
         }}};

const std::unordered_map<ComponentId, ComponentCreator> ComponentFactory::componentCreator = {
    {OtherComponent::TCP_SOURCE_0,
     [](Context& ctx) {
         return std::make_shared<TcpSource>(OtherComponent::TCP_SOURCE_0, *ctx.bus);
     }},
    {OtherComponent::TCP_COMMAND_DECODER_0,
     [](Context& ctx) {
         return std::make_shared<TcpCommandDecoder>(OtherComponent::TCP_COMMAND_DECODER_0,
                                                    *ctx.bus);
     }},
    {OtherComponent::GPS_DRIVER_0,
     [](Context& ctx) {
         return std::make_shared<SerialUblox>(OtherComponent::GPS_DRIVER_0, *ctx.io,
                                              ctx.config->gpsdevname, ctx.config->gnss_baudrate,
                                              *ctx.bus);
     }},
    {OtherComponent::GPIO_DRIVER_0, [](Context& ctx) {
         return std::make_shared<GpioDriver>(OtherComponent::GPIO_DRIVER_0, ctx.config->gpiodevname,
                                             *ctx.bus);
     }}};
