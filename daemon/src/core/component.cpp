#include "core/component.h"
#include "core/logging/logger.h"

#include <algorithm>
#include <type_traits>


Component::Component(const ComponentId id) : id_{id} {}

auto Component::id() const noexcept -> ComponentId
{
    return id_;
}

void Component::handleDeviceMissing(const ComponentId id)
{
    auto name = std::visit([](auto&& arg) -> std::string
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, NonDeviceComponent>) {
            return std::to_string(static_cast<unsigned>(arg));
        }
        else if constexpr (std::is_same_v<T, Device>) {
            return std::to_string(static_cast<unsigned>(arg));
        }
        else {
            static_assert(false, "non-exhaustive visitor!");
        }
    }, id);
    auto it = std::find_if(std::begin(componentLookup), std::end(componentLookup),
                        [&id](auto&& p) { return p.second == id; });
    if (it != componentLookup.end()){
        name = it->first;
    }
    logError("Failed to find device " + name);
}
