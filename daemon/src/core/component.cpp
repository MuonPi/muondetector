#include "core/component.h"

Component::Component(ComponentId id) : m_id{id} {}

auto Component::id() -> ComponentId
{
    return m_id;
}