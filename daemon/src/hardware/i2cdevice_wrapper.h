#ifndef I2CDEVICE_WRAPPER_H
#define I2CDEVICE_WRAPPER_H

#include "hardware/idevice.h"

#include <memory>

template<typename TDevice>
class I2CDeviceWrapper : public IDevice
{
public:
    I2CDeviceWrapper(std::unique_ptr<TDevice> && dev) : m_dev {std::move(dev)} {}
    TDevice& device() { return *m_dev; }

private:
    std::unique_ptr<TDevice> m_dev;
};

#endif // I2CDEVICE_WRAPPER_H