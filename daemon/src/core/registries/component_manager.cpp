#include "core/registries/component_manager.h"

#include "core/component.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

void ComponentManager::add(ComponentId id, std::shared_ptr<Component> src) {
    m_components[id] = std::move(src);
}
