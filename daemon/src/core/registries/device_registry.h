#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include <memory>
#include <unordered_map>
#include <mutex>
#include <typeindex>

#include "hardware/i2cdevices.h"

class DeviceRegistry
{
public:
    template<typename T, typename... Args>
    T& emplace(uint32_t id, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *ptr;

        m_devices[id] = std::move(ptr);
        m_types.insert_or_assign(id, std::type_index(typeid(T)));

        return ref;
    }

    template<typename T>
    T* get(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_devices.find(id);
        if (it == m_devices.end())
            return nullptr;

        return dynamic_cast<T*>(it->second.get());
    }

    i2cDevice* getBase(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_devices.find(id);
        return (it != m_devices.end()) ? it->second.get() : nullptr;
    }

private:
    std::unordered_map<uint32_t, std::unique_ptr<i2cDevice>> m_devices;
    std::unordered_map<uint32_t, std::type_index> m_types;
    std::mutex m_mutex;
};

#endif // SENSOR_REGSITRY_H