#include "core/registries/component_manager.h"
#include "core/component.h"

#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>


void ComponentManager::add(ComponentId id, std::shared_ptr<Component> src)
{
    m_components[id] = std::move(src);
}
