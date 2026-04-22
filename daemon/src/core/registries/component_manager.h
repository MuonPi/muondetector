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

  private:
    std::mutex m_mutex;
    std::unordered_map<ComponentId, std::shared_ptr<Component>> m_components;
};

#endif // SOURCE_MANAGER_H^