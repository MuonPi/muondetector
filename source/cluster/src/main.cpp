#include "log.h"
#include "core.h"
#include "detectortracker.h"

#include "mqtteventsource.h"
#include "mqttlogsource.h"
#include "mqttlink.h"
#include "statesupervisor.h"

#define CLUSTER_RUN_SERVER

#ifdef CLUSTER_RUN_SERVER
#include "databaseeventsink.h"
#include "databaselink.h"
#endif

#include "asciieventsink.h"
#include "asciilogsink.h"

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
    MuonPi::Log::Log::singleton()->add_sink(std::make_shared<MuonPi::Log::StreamSink>(std::cout));
    MuonPi::Log::Log::singleton()->add_sink(std::make_shared<MuonPi::Log::SyslogSink>());


    MuonPi::MqttLink::LoginData login;
    login.username = "benjamin";
    login.password = "goodpassword";
    login.station_id = "ds9";

    MuonPi::MqttLink source_link {"116.202.96.181:1883", login};
//    MuonPi::MqttLink source_link {"168.119.243.171:1883", login};

    if (!source_link.startup()) {
        return -1;
    }

    MuonPi::MqttEventSource::Subscribers source_topics{
        source_link.subscribe("muonpi/data/#", "muonpi/data/[/a-zA-Z0-9_-]+"),
        source_link.subscribe("muonpi/l1data/#", "muonpi/l1data/[/a-zA-Z0-9_-]+")
    };


    auto log_source { std::make_shared<MuonPi::MqttLogSource>(source_link.subscribe("muonpi/log/#", "muonpi/log/[/a-zA-Z0-9_-]+")) };

    auto event_source { std::make_shared<MuonPi::MqttEventSource>(std::move(source_topics)) };
    auto ascii_log_sink { std::make_shared<MuonPi::AsciiLogSink>(std::cout) };


    MuonPi::StateSupervisor supervisor{{ascii_log_sink}};

    MuonPi::DetectorTracker detector_tracker{{log_source}, supervisor};


#ifdef CLUSTER_RUN_SERVER
    MuonPi::DatabaseLink db_link {"", {"", ""}, ""};
    auto event_sink { std::make_shared<MuonPi::DatabaseEventSink>(db_link) };
#else
    /*
    MuonPi::MqttLink sink_link {"", login};
    MuonPi::MqttEventSink::Publishers sink_topics;

    sink_topics.single = sink_link.publish("muonpi/events/...");
    sink_topics.combined = sink_link.publish("muonpi/l1data/...");
    auto event_sink { std::make_shared<MuonPi::MqttEventSink>(std::move(sink_topics)) };
*/

#endif

    auto ascii_event_sink { std::make_shared<MuonPi::AsciiEventSink>(std::cout) };
    supervisor.add_thread(&detector_tracker);
    supervisor.add_thread(&source_link);
    supervisor.add_thread(log_source.get());
    supervisor.add_thread(event_source.get());
    supervisor.add_thread(ascii_event_sink.get());
    supervisor.add_thread(ascii_log_sink.get());

    source_link.start();

    MuonPi::Core core{{event_sink, ascii_event_sink}, {event_source}, detector_tracker, supervisor};

    shutdown_handler = [&](int signal) {
        if (signal == SIGINT) {
            core.stop();
        }
    };

    std::signal(SIGINT, signal_handler);

    core.start_synchronuos();

    source_link.stop();
    return core.wait();
}
