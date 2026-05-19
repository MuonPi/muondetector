#include "device_factory.h"

#include "core/logging/logger.h"
#include "hardware/devices.h"
#include "hardware/i2c/i2cutil.h"
#include "hardware/i2cdevice_wrapper.h"
#include "hardware/i2cdevices.h"
#include "utility/conversion.h"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

void DeviceFactory::i2cReset() {
    // reset the I2C bus by issuing a general call reset
    I2cGeneralCall::resetDevices();
}

void DeviceFactory::i2cInfo() {
    std::stringstream sstr;
    sstr << "Nr. of invoked I2C devices (plain count): " << std::dec << i2cDevice::getNrDevices()
         << std::endl;
    sstr << "Nr. of invoked I2C devices (gl. device list's size): "
         << i2cDevice::getGlobalDeviceList().size() << std::endl;
    sstr << "Nr. of bytes read on I2C bus: " << i2cDevice::getGlobalNrBytesRead() << std::endl;
    sstr << "Nr. of bytes written on I2C bus: " << i2cDevice::getGlobalNrBytesWritten()
         << std::endl;
    sstr << "list of device addresses: " << std::endl;
    for (uint8_t i = 0; i < i2cDevice::getGlobalDeviceList().size(); i++) {
        sstr << (int) i + 1 << " 0x" << std::hex
             << (int) i2cDevice::getGlobalDeviceList()[i]->getAddress() << " "
             << i2cDevice::getGlobalDeviceList()[i]->getTitle();
        if (i2cDevice::getGlobalDeviceList()[i]->devicePresent())
            sstr << " present" << std::endl;
        else
            sstr << " missing" << std::endl;
    }
    // if (temp_sensor_p)
    //     dynamic_cast<i2cDevice*>(temp_sensor_p.get())->getCapabilities();
    logDebug(sstr.str());
}

auto DeviceFactory::create(const DeviceConfig& config) -> std::unique_ptr<IDevice> {
    auto it = DeviceFactory::deviceCreator.find(config.id);
    if (it == DeviceFactory::deviceCreator.end()) {
        throw std::runtime_error("No factory method found for creation of device " +
                                 std::to_string(static_cast<unsigned>(config.id)));
    }
    return it->second(config);
}

void DeviceFactory::deviceNotFoundError(const std::string& name, std::uint8_t address) {
    logWarn(name + " device NOT found! Address: " + std::to_string(address));
}

const std::unordered_map<Device, DeviceCreator> DeviceFactory::deviceCreator = {
    {Device::ADS1115_0,
     [](const DeviceConfig& cfg) -> std::unique_ptr<I2CDeviceWrapper<ADS1115>> {
         auto device_p = std::make_unique<ADS1115>(cfg.device.value().c_str(), cfg.address.value());
         if (!device_p->probeDevicePresence() || !device_p->identify()) {
             deviceNotFoundError(device_p->getName(), cfg.address.value());
             return nullptr;
         }
         logInfo("adc " + device_p->getName() + " identified at 0x" +
                 to_hex(device_p->getAddress()));
         return std::make_unique<I2CDeviceWrapper<ADS1115>>(std::move(device_p));
     }},
    {Device::MCP4728_0,
     [](const DeviceConfig& cfg) -> std::unique_ptr<I2CDeviceWrapper<MCP4728>> {
         auto device_p = std::make_unique<MCP4728>(cfg.device.value().c_str(), cfg.address.value());
         if (!device_p->probeDevicePresence() || !device_p->identify()) {
             deviceNotFoundError(device_p->getName(), cfg.address.value());
             return nullptr;
         }
         logInfo("dac " + device_p->getName() + " identified at 0x" +
                 to_hex(device_p->getAddress()));
         return std::make_unique<I2CDeviceWrapper<MCP4728>>(std::move(device_p));
     }},
    {Device::MIC184_0,
     [](const DeviceConfig& cfg) -> std::unique_ptr<I2CDeviceWrapper<MIC184>> {
         auto device_p = std::make_unique<MIC184>(cfg.device.value().c_str(), cfg.address.value());
         if (!device_p->probeDevicePresence() || !device_p->identify()) {
             deviceNotFoundError(device_p->getName(), cfg.address.value());
             return nullptr;
         }
         logInfo("temp " + device_p->getName() + " identified at 0x" +
                 to_hex(device_p->getAddress()));
         return std::make_unique<I2CDeviceWrapper<MIC184>>(std::move(device_p));
     }},
    {Device::LM75_0,
     [](const DeviceConfig& cfg) -> std::unique_ptr<I2CDeviceWrapper<LM75>> {
         auto device_p = std::make_unique<LM75>(cfg.device.value().c_str(), cfg.address.value());
         if (!device_p->probeDevicePresence() || !device_p->identify()) {
             deviceNotFoundError(device_p->getName(), cfg.address.value());
             return nullptr;
         }
         logInfo("temp " + device_p->getName() + " identified at 0x" +
                 to_hex(device_p->getAddress()));
         return std::make_unique<I2CDeviceWrapper<LM75>>(std::move(device_p));
     }},
    {Device::EEPROM24AA02_0,
     [](const DeviceConfig& cfg) -> std::unique_ptr<I2CDeviceWrapper<EEPROM24AA02>> {
         auto device_p =
             std::make_unique<EEPROM24AA02>(cfg.device.value().c_str(), cfg.address.value());
         if (!device_p->probeDevicePresence() || !device_p->identify()) {
             deviceNotFoundError(device_p->getName(), cfg.address.value());
             return nullptr;
         }
         logInfo("eeprom " + device_p->getName() + " identified at 0x" +
                 to_hex(device_p->getAddress()));
         return std::make_unique<I2CDeviceWrapper<EEPROM24AA02>>(std::move(device_p));
     }},
    {Device::PCA9536_0,
     [](const DeviceConfig& cfg) -> std::unique_ptr<I2CDeviceWrapper<PCA9536>> {
         auto device_p = std::make_unique<PCA9536>(cfg.device.value().c_str(), cfg.address.value());
         if (!device_p->probeDevicePresence() || !device_p->identify()) {
             deviceNotFoundError(device_p->getName(), cfg.address.value());
             return nullptr;
         }
         logInfo("io expander " + device_p->getName() + " identified at 0x" +
                 to_hex(device_p->getAddress()));
         return std::make_unique<I2CDeviceWrapper<PCA9536>>(std::move(device_p));
     }},
    {Device::UBLOX_I2C_0,
     [](const DeviceConfig& cfg) -> std::unique_ptr<I2CDeviceWrapper<UbloxI2c>> {
         auto device_p =
             std::make_unique<UbloxI2c>(cfg.device.value().c_str(), cfg.address.value());
         if (!device_p->devicePresent()) {
             deviceNotFoundError("Ublox", cfg.address.value());
             return nullptr;
         }
         device_p->lock();
         logInfo("Ublox device identified at 0x" + to_hex(device_p->getAddress()));
         return std::make_unique<I2CDeviceWrapper<UbloxI2c>>(std::move(device_p));
     }},
    {Device::ADAFRUIT_SSD1306_0,
     [](const DeviceConfig& cfg) -> std::unique_ptr<I2CDeviceWrapper<Adafruit_SSD1306>> {
         auto device_p =
             std::make_unique<Adafruit_SSD1306>(cfg.device.value().c_str(), cfg.address.value());
         if (!device_p->devicePresent()) {
             deviceNotFoundError("Adafruit SSD1306 OLED", cfg.address.value());
             return nullptr;
         }
         logInfo("I2C SSD1306-type OLED display identified at 0x" + to_hex(device_p->getAddress()));
         return std::make_unique<I2CDeviceWrapper<Adafruit_SSD1306>>(std::move(device_p));
     }}};