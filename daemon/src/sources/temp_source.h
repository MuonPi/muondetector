#ifndef TEMP_SOURCE_H
#define TEMP_SOURCE_H

#include "core/event_bus.h"
#include "sources/source.h"

#include <memory>

class TempSource : public Source {
  public:
    explicit TempSource(ComponentId id, EventBus& bus);

    void update() override;

  private:
    EventBus& bus_;
};

#endif // TEMP_SOURCE_H