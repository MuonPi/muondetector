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
#include "databaselogsink.h"
#include "databaselink.h"
#endif

#include "asciieventsink.h"
#include "asciilogsink.h"

#include "detectorlog.h"
#include "clusterlog.h"

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

    MuonPi::MqttLink source_link {login, "116.202.96.181", 1883};

//    MuonPi::MqttLink source_link {login, "168.119.243.171", 1883};



    if (!source_link.wait_for(MuonPi::MqttLink::Status::Connected, std::chrono::seconds{5})) {
        return -1;
    }
    MuonPi::MqttEventSource::Subscribers source_topics{
        source_link.subscribe("muonpi/data/#"),
        source_link.subscribe("muonpi/l1data/#")
    };


    auto log_source { std::make_shared<MuonPi::MqttLogSource>(source_link.subscribe("muonpi/log/#")) };

    auto event_source { std::make_shared<MuonPi::MqttEventSource>(std::move(source_topics)) };
    auto ascii_log_sink { std::make_shared<MuonPi::AsciiLogSink>(std::cout) };


#ifdef CLUSTER_RUN_SERVER
//    MuonPi::DatabaseLink db_link {"", {"", ""}, ""};
//    auto event_sink { std::make_shared<MuonPi::DatabaseEventSink>(db_link) };
//    auto clusterlog_sink { std::make_shared<MuonPi::DatabaseLogSink<MuonPi::ClusterLog>>(db_link) };
#else
    /*
    MuonPi::MqttLink sink_link {"", login};
    MuonPi::MqttEventSink::Publishers sink_topics;

    sink_topics.single = sink_link.publish("muonpi/events/...");
    sink_topics.combined = sink_link.publish("muonpi/l1data/...");
    auto event_sink { std::make_shared<MuonPi::MqttEventSink>(std::move(sink_topics)) };
*/

#endif
    MuonPi::StateSupervisor supervisor{{ascii_log_sink}};

//    auto detectorlog_sink { std::make_shared<MuonPi::DatabaseLogSink<MuonPi::DetectorLog>>(db_link) };

    MuonPi::DetectorTracker detector_tracker{{log_source}, {}, supervisor};

    auto ascii_event_sink { std::make_shared<MuonPi::AsciiEventSink>(std::cout) };
    supervisor.add_thread(&detector_tracker);
    supervisor.add_thread(&source_link);
    supervisor.add_thread(log_source.get());
    supervisor.add_thread(event_source.get());
    supervisor.add_thread(ascii_event_sink.get());
    supervisor.add_thread(ascii_log_sink.get());

#ifdef CLUSTER_RUN_SERVER
    MuonPi::Core core{{ascii_event_sink}, {event_source}, detector_tracker, supervisor};
#else
    MuonPi::Core core{{ascii_event_sink}, {event_source}, detector_tracker, supervisor};
#endif

    shutdown_handler = [&](int signal) {
        if (
                   (signal == SIGINT)
                || (signal == SIGTERM)
                || (signal == SIGHUP)
                ) {
            MuonPi::Log::notice()<<"Received signal: " + std::to_string(signal) + ". Exiting.";
            supervisor.stop();
        }
        if (
                   signal == SIGSEGV
                ) {
            MuonPi::Log::critical()<<"Received signal: " + std::to_string(signal) + ". Exiting.";
            supervisor.stop();
            core.stop();
        }
    };

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGHUP, signal_handler);
    std::signal(SIGSEGV, signal_handler);

    core.start_synchronuos();

    return core.wait();
}
