#ifndef SOURCE_H
#define SOURCE_H

#include <cstdint>
#include <variant>
#include "core/component.h"
#include "hardware/devices.h"

class Source : public Component
{
public:
    using Component::Component;
    virtual ~Source() = default;
    virtual void update() = 0;
};

#endif // SOURCE_H