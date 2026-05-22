#include "core/maintenance.h"

#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/data_store.h"
#include "core/scheduler.h"
// #include "hardware/ublox/serialublox.h"
#include "data/commands/ubx_msg_poll_rate_cmd.h"

#include <chrono>
#include <memory>

void Maintenance::registerEnsureData(EventBus& bus, DataStore& datastore, Scheduler& scheduler) {

    // Check if specific structs are in datastore and poll them if not
    const auto interval = std::chrono::seconds(2);

    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<NavStatus>(bus, datastore);
    });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<UbxDopStruct>(bus, datastore);
    });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<NavTimeUTC>(bus, datastore);
    });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<NavClock>(bus, datastore);
    });
    scheduler.every(
        interval, [&bus, &datastore]() { return Maintenance::ensureData<NavSat>(bus, datastore); });
    scheduler.every(
        interval, [&bus, &datastore]() { return Maintenance::ensureData<CfgAnt>(bus, datastore); });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<CfgNavX5>(bus, datastore);
    });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<CfgNav5>(bus, datastore);
    });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<UbxTimePulseStruct>(bus, datastore);
    });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<CfgGNSS>(bus, datastore);
    });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<CfgNavX5>(bus, datastore);
    });
    scheduler.every(
        interval, [&bus, &datastore]() { return Maintenance::ensureData<MonRx>(bus, datastore); });
    scheduler.every(
        interval, [&bus, &datastore]() { return Maintenance::ensureData<MonTx>(bus, datastore); });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<GnssMonHwStruct>(bus, datastore);
    });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<GnssMonHw2Struct>(bus, datastore);
    });
    scheduler.every(interval, [&bus, &datastore]() {
        return Maintenance::ensureData<GpsVersion>(bus, datastore);
    });

    scheduler.every(interval, [&bus, &datastore]() {
        static const auto program_start = std::chrono::steady_clock::now();
        auto runtime = std::chrono::steady_clock::now() - program_start;

        if (runtime < initialization_time) {
            return true; // re-schedule without action due to un-initialized state
        }
        auto* rates = datastore.get<std::unordered_map<std::uint16_t, CfgMsg>>();
        if (rates == nullptr) {
            datastore.store_if_empty(std::unordered_map<std::uint16_t, CfgMsg>{});
            rates = datastore.get<std::unordered_map<std::uint16_t, CfgMsg>>();
            if (rates == nullptr) {
                throw std::runtime_error("Data empty after store");
            }
        }
        std::unordered_set<UBX_MSG::msg_id> existing{};
        for (const auto& [key, entry] : *rates) {
            existing.emplace(static_cast<UBX_MSG::msg_id>(key));
        }

        bool all_exist = true;
        for (const auto& [key, entry] : defaultRates) {
            if (existing.contains(key) == false) {
                all_exist = false;
                logInfo("Polling UBX message rate " + std::to_string(static_cast<unsigned>(key)));

                bus.publish(UbxMsgPollRateCmd{key});
            }
        }
        return !all_exist; // if not all rates exist in config: continue scheduling
    });
}

void Maintenance::datastoreMaintenance(EventBus& bus, DataStore& datastore) {
    static const auto program_start = std::chrono::steady_clock::now();
    auto runtime = std::chrono::steady_clock::now() - program_start;

    if (runtime < initialization_time) {
        return;
    }
    // If requested, do some data invlidation or refreshing logic here.
}