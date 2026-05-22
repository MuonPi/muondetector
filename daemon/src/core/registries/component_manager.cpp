#include "core/registries/component_manager.h"

#include "core/component.h"

#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

void ComponentManager::add(ComponentId id, std::shared_ptr<Component> src) {
    m_components[id] = std::move(src);
}

auto ComponentManager::contains(ComponentId id) -> bool {

    return m_components.contains(id);
}

auto ComponentManager::report() -> std::string {
    std::stringstream sstr;
    sstr << "Components registered:\n";
    for (auto& [key, value] : m_components) {
        sstr << value->name() << "\n";
    }
    return sstr.str();
}