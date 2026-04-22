#ifndef SOURCE_H
#define SOURCE_H

#include "core/component.h"
#include "hardware/devices.h"

#include <cstdint>
#include <variant>

class Source : public Component {
  public:
    using Component::Component;
    virtual ~Source() = default;
    virtual void update() = 0;
};

#endif // SOURCE_H