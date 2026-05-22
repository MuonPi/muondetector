#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/data_store.h"
#include "core/scheduler.h"
// #include "hardware/ublox/serialublox.h"
#include "data/commands/ubx_msg_poll_cmd.h"
#include "data/events/ubx_event.h"
#include "data/ublox/ublox_messages.h"

#include <chrono>
#include <memory>

class Maintenance {
  public:
    template <typename T>
    inline static auto ensureData(EventBus& bus, DataStore& datastore) -> bool {
        static const auto program_start = std::chrono::steady_clock::now();
        auto runtime = std::chrono::steady_clock::now() - program_start;

        if (runtime < initialization_time) {
            return true; // re-schedule without action due to un-initialized state
        }
        if (datastore.lastUpdate<T>().has_value()) {
            return false; // cancels re-scheduling because data exists now
        }

        logInfo("Polling UBX message " + std::to_string(static_cast<unsigned>(MsgId<T>::value)));

        bus.publish(UbxMsgPollCmd{MsgId<T>::value});
        return true;
    }
    static void registerEnsureData(EventBus& bus, DataStore& datastore, Scheduler& scheduler);
    static void datastoreMaintenance(EventBus& bus, DataStore& datastore);

  private:
    // waiting time before first maintenance can be done
    inline static constexpr auto initialization_time = std::chrono::seconds(3);
    Maintenance() = delete;
};

#endif // MAINTENANCE_H