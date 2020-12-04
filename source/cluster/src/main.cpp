#include "core.h"
#include "detectortracker.h"

#include "mqtteventsource.h"
#include "mqttlogsource.h"

#ifdef CLUSTER_RUN_SERVER
#include "databaseeventsink.h"
#else
#include "mqtteventsink.h"
#endif

auto main() -> int
{
    auto log_source { std::make_unique<MuonPi::MqttLogSource>() };

    auto detector_tracker { std::make_unique<MuonPi::DetectorTracker>(std::move(log_source)) };

    auto event_source { std::make_unique<MuonPi::MqttEventSource>() };
#ifdef CLUSTER_RUN_SERVER
    auto event_sink { std::make_unique<MuonPi::DatabaseEventSink>() };
#else
    auto event_sink { std::make_unique<MuonPi::MqttEventSink>() };
#endif

    MuonPi::Core core{std::move(event_sink), std::move(event_source), std::move(detector_tracker)};

    return core.wait();
}
