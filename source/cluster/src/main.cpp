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
    MuonPi::Log::Log::singleton()->add_sink(std::make_shared<MuonPi::Log::StreamSink>(std::cerr));
    MuonPi::Log::Log::singleton()->add_sink(std::make_shared<MuonPi::Log::SyslogSink>());


    MuonPi::MqttLink::LoginData login;
    login.username = "benjamin";
    login.password = "goodpassword";
    login.station_id = "ds9";

    MuonPi::MqttLink source_link {login, "116.202.96.181", 1883};

    if (!source_link.wait_for(MuonPi::MqttLink::Status::Connected, std::chrono::seconds{5})) {
        return -1;
    }


    MuonPi::MqttEventSource::Subscribers source_topics{
        source_link.subscribe("muonpi/data/#"),
        source_link.subscribe("muonpi/l1data/#")
    };
    auto event_source { std::make_shared<MuonPi::MqttEventSource>(std::move(source_topics)) };
    auto log_source { std::make_shared<MuonPi::MqttLogSource>(source_link.subscribe("muonpi/log/#")) };

    auto ascii_clusterlog_sink { std::make_shared<MuonPi::AsciiLogSink<MuonPi::ClusterLog>>(std::cout) };
    auto ascii_detectorlog_sink { std::make_shared<MuonPi::AsciiLogSink<MuonPi::DetectorLog>>(std::cout) };
    auto ascii_event_sink { std::make_shared<MuonPi::AsciiEventSink>(std::cout) };


#ifdef CLUSTER_RUN_SERVER
    MuonPi::DatabaseLink db_link {"", {"", ""}, ""};

    auto event_sink { std::make_shared<MuonPi::DatabaseEventSink>(db_link) };
    auto clusterlog_sink { std::make_shared<MuonPi::DatabaseLogSink<MuonPi::ClusterLog>>(db_link) };
    auto detectorlog_sink { std::make_shared<MuonPi::DatabaseLogSink<MuonPi::DetectorLog>>(db_link) };

#else
    MuonPi::MqttLink sink_link {login, "116.202.96.181", 1883};

    if (!sink_link.wait_for(MuonPi::MqttLink::Status::Connected, std::chrono::seconds{5})) {
        return -1;
    }

    MuonPi::MqttEventSink::Publishers sink_topics {
        sink_link.publish("muonpi/events/MuonPi");
        sink_link.publish("muonpi/l1data/MuonPi");
    };

    auto event_sink { std::make_shared<MuonPi::MqttEventSink>(std::move(sink_topics)) };
    auto clusterlog_sink { std::make_shared<MuonPi::MqttLogSink<MuonPi::ClusterLog>>(sink_link.publish("muonpi/log/cluster/MuonPi")) };
    auto detectorlog_sink { std::make_shared<MuonPi::MqttLogSink<MuonPi::DetectorLog>>(sink_link.publish("muonpi/log/detector/MuonPi")) };

#endif

    MuonPi::StateSupervisor supervisor{{ascii_clusterlog_sink, clusterlog_sink}};
    MuonPi::DetectorTracker detector_tracker{{log_source}, {ascii_detectorlog_sink, detectorlog_sink}, supervisor};
    MuonPi::Core core{{ascii_event_sink, event_sink}, {event_source}, detector_tracker, supervisor};

#ifndef CLUSTER_RUN_SERVER
    supervisor.add_thread(&sink_link);
#endif
    supervisor.add_thread(&detector_tracker);
    supervisor.add_thread(&source_link);
    supervisor.add_thread(detectorlog_sink.get());
    supervisor.add_thread(log_source.get());
    supervisor.add_thread(event_source.get());
    supervisor.add_thread(ascii_event_sink.get());
    supervisor.add_thread(ascii_clusterlog_sink.get());
    supervisor.add_thread(ascii_detectorlog_sink.get());


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
