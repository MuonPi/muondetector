#include "core.h"
#include "detectortracker.h"

#include "mqtteventsource.h"
#include "mqttlogsource.h"
#include "mqttlink.h"

#ifdef CLUSTER_RUN_SERVER
#include "databaseeventsink.h"
#else
#include "mqtteventsink.h"
#endif

auto main() -> int
{
    MuonPi::MqttLink::LoginData login;
    login.username = "benjamin";
    login.password = "goodpassword";
    login.station_id = "ds9";

    MuonPi::MqttLink source_link {"", login};

    MuonPi::MqttEventSource::Subscribers source_topics;

    source_topics.single = source_link.subscribe("muonpi/events/...");
    source_topics.combined = source_link.subscribe("muonpi/l1data/...");

    auto log_source { std::make_unique<MuonPi::MqttLogSource>(source_link.subscribe("muonpi/log/...")) };
    auto event_source { std::make_unique<MuonPi::MqttEventSource>(std::move(source_topics)) };

    auto detector_tracker { std::make_unique<MuonPi::DetectorTracker>(std::move(log_source)) };

#ifdef CLUSTER_RUN_SERVER
    auto event_sink { std::make_unique<MuonPi::DatabaseEventSink>() };
#else
    MuonPi::MqttLink sink_link {"", login};
    MuonPi::MqttEventSink::Publishers sink_topics;

    sink_topics.single = sink_link.publish("muonpi/events/...");
    sink_topics.combined = sink_link.publish("muonpi/l1data/...");

    auto event_sink { std::make_unique<MuonPi::MqttEventSink>(std::move(sink_topics)) };
#endif

    MuonPi::Core core{std::move(event_sink), std::move(event_source), std::move(detector_tracker)};

    return core.wait();
}
