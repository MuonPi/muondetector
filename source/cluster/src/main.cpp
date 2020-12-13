#include "log.h"
#include "core.h"
#include "detectortracker.h"

#include "mqtteventsource.h"
#include "mqttlogsource.h"
#include "mqttlink.h"

#ifdef CLUSTER_RUN_SERVER
#include "databaseeventsink.h"
#else
#include "mqtteventsink.h"
#include "asciieventsink.h"
#endif

#include <csignal>
#include <functional>

void signal_handler(int signal);


static std::function<void(int)> shutdown_handler;

void signal_handler(int signal)
{
    shutdown_handler(signal);
}

auto main() -> int
{
    MuonPi::Log::init(MuonPi::Log::Level::Debug);

    MuonPi::Log::Log::singleton()->add_sink(std::make_shared<MuonPi::Log::StreamSink>(std::cout));

    MuonPi::MqttLink::LoginData login;
    login.username = "benjamin";
    login.password = "goodpassword";
    login.station_id = "ds9";

    MuonPi::MqttLink source_link {"116.202.96.181:1883", login};
//    MuonPi::MqttLink source_link {"168.119.243.171:1883", login};

    if (!source_link.wait_for(MuonPi::MqttLink::Status::Connected, std::chrono::seconds{5})) {
        return -1;
    }

    MuonPi::MqttEventSource::Subscribers source_topics;

    source_topics.single = source_link.subscribe("muonpi/data/#", "muonpi/data/[/a-zA-Z0-9]+");
    source_topics.combined = source_link.subscribe("muonpi/l1data/#", "muonpi/l1data/[/a-zA-Z0-9]+");

    auto log_source { std::make_unique<MuonPi::MqttLogSource>(source_link.subscribe("muonpi/log/#", "muonpi/log/[/a-zA-Z0-9]+")) };

    auto event_source { std::make_unique<MuonPi::MqttEventSource>(std::move(source_topics)) };

    auto detector_tracker { std::make_unique<MuonPi::DetectorTracker>(std::move(log_source)) };

    source_link.startup();

#ifdef CLUSTER_RUN_SERVER
    auto event_sink { std::make_unique<MuonPi::DatabaseEventSink>() };
#else
    /*
    MuonPi::MqttLink sink_link {"", login};
    MuonPi::MqttEventSink::Publishers sink_topics;

    sink_topics.single = sink_link.publish("muonpi/events/...");
    sink_topics.combined = sink_link.publish("muonpi/l1data/...");

    auto event_sink { std::make_unique<MuonPi::MqttEventSink>(std::move(sink_topics)) };*/

    auto event_sink { std::make_unique<MuonPi::AsciiEventSink>(std::cout) };
#endif

    MuonPi::Core core{std::move(event_sink), std::move(event_source), std::move(detector_tracker)};

    shutdown_handler = [&](int signal) {
        if (signal == SIGINT) {
            core.stop();
        }
    };

    std::signal(SIGINT, signal_handler);


    core.join();

    source_link.stop();
    return core.wait();
}
