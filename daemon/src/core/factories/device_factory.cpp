#include "device_factory.h"

#include "core/logging/logger.h"
#include "hardware/devices.h"
#include "hardware/i2c/ads1115.h"
#include "hardware/i2c/eeprom24aa02.h"
#include "hardware/i2c/i2cutil.h"
#include "hardware/i2cdevice_wrapper.h"

#include <cstdint>
#include <memory>
#include <string>

void DeviceFactory::i2cReset() {
    // reset the I2C bus by issuing a general call reset
    I2cGeneralCall::resetDevices();
}

auto DeviceFactory::create(const DeviceConfig& config) -> std::unique_ptr<IDevice> {
    auto it = DeviceFactory::deviceCreator.find(config.id);
    if (it == DeviceFactory::deviceCreator.end()) {
        throw std::runtime_error("No factory method found for creation of device " +
                                 std::to_string(static_cast<unsigned>(config.id)));
    }
    return it->second(config);
}

const std::unordered_map<Device, DeviceCreator> DeviceFactory::deviceCreator = {
    {Device::ADS1115_0,
     [](const DeviceConfig& cfg) {
         return std::make_unique<I2CDeviceWrapper<ADS1115>>(
             std::make_unique<ADS1115>(cfg.device.value().c_str(), cfg.address.value()));
     }},
    {Device::MCP4728_0,
     [](const DeviceConfig& cfg) {
         return std::make_unique<I2CDeviceWrapper<MCP4728>>(
             std::make_unique<MCP4728>(cfg.device.value().c_str(), cfg.address.value()));
     }},
    {Device::EEPROM24AA02_0, [](const DeviceConfig& cfg) {
         // std::set<uint8_t> possible_addresses { 0x50 };
         // auto found_dev_addresses = findI2cDeviceType<EEPROM24AA02>(possible_addresses);
         // std::shared_ptr<EEPROM24AA02> eep24aa02_p;
         // if (found_dev_addresses.size() > 0) {
         //     eep24aa02_p = std::make_shared<EEPROM24AA02>(found_dev_addresses.front());
         // }
         // if (eep24aa02_p->identify()) {
         //     eep24aa02_p =
         //     std::static_pointer_ast<DeviceFunction<DeviceType::EEPROM>>(eep24aa02_p); std::cout
         //     << "eeprom " << eep24aa02_p->getName() << " identified at 0x" << std::hex <<
         //     (int)eep24aa02_p->getAddress() << std::endl;
         // } else {
         //     eep24aa02_p.reset();
         //     logError("eeprom device NOT found!");
         // }
         return std::make_unique<I2CDeviceWrapper<EEPROM24AA02>>(
             std::make_unique<EEPROM24AA02>(cfg.device.value().c_str(), cfg.address.value()));
     }}};