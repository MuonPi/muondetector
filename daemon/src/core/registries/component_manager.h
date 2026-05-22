#ifndef SOURCE_MANAGER_H
#define SOURCE_MANAGER_H

#include "core/component.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class ComponentManager {
  public:
    void add(ComponentId id, std::shared_ptr<Component> src);

    template <typename T>
    T* get(ComponentId id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_components.find(id);
        if (it == m_components.end()) {
            return nullptr;
        }

        return dynamic_cast<T*>(it->second.get());
    }

    auto contains(ComponentId id) -> bool;
    // template <typename T>
    // std::weak_ptr<T> getWeak(ComponentId id) {
    //     std::lock_guard<std::mutex> lock(m_mutex);

    //     auto it = m_components.find(id);
    //     if (it == m_components.end()) {
    //         return {};
    //     }

    //     return std::dynamic_pointer_cast<T>(it->second);
    // }
    auto report() -> std::string;

  private:
    std::mutex m_mutex;
    std::unordered_map<ComponentId, std::shared_ptr<Component>> m_components;
};

#endif // SOURCE_MANAGER_H^