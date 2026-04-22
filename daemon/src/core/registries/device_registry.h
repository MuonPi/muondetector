#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include "hardware/devices.h"
#include "hardware/idevice.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>

class DeviceRegistry {
  public:
    void add(Device id, std::unique_ptr<IDevice> dev) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_devices[id] = std::move(dev);
    }

    template <typename T>
    T* get(Device id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_devices.find(id);
        if (it == m_devices.end())
            return nullptr;

        return dynamic_cast<T*>(it->second.get());
    }

  private:
    std::unordered_map<Device, std::unique_ptr<IDevice>> m_devices;
    std::mutex m_mutex;
};
#endif // SENSOR_REGSITRY_H