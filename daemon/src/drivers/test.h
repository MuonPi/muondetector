#ifndef FIX_PROCESSOR_H
#define FIX_PROCESSOR_H

#include "core/event_bus.h"
#include "datastore/datastore.h"

class FixProcessor {
  public:
    FixProcessor(EventBus& bus, Datastore& store) : bus_(bus), store_(store) {
        bus_.subscribe<NavSat>([this](const NavSat&) { evaluate(); });
        bus_.subscribe<NavTimeGPS>([this](const NavTimeGPS&) { evaluate(); });
    }

  private:
    void evaluate() {
        auto sat = store_.get<NavSat>();
        auto time = store_.get<NavTimeGPS>();

        if (!sat || !time)
            return;

        // optional: check age
        auto satAge = store_.age<NavSat>();
        auto timeAge = store_.age<NavTimeGPS>();

        if (*satAge > 500ms || *timeAge > 500ms)
            return;

        // compute derived result
        bool validFix = sat->first.numSats >= 4;

        bus_.publish(validFix); // or a struct
    }

    EventBus& bus_;
    Datastore& store_;
};

#endif // FIX_PROCESSOR_H