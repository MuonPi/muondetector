#include "databaseeventsink.h"

#include "event.h"
#include "databaselink.h"
#include "utility.h"
#include "log.h"

#include <sstream>

namespace MuonPi {

DatabaseEventSink::DatabaseEventSink(DatabaseLink& link)
    : m_link { link }
{
    start();
}

auto DatabaseEventSink::step() -> int
{
    if (has_items()) {
        process(next_item());
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

void DatabaseEventSink::process(Event event)
{
    if (event.n() == 1) {
        // by default, don't write the single events to the db
        return;
    }

    const std::int64_t cluster_coinc_time = event.end() - event.start();
    GUID guid{event.hash(), static_cast<std::uint64_t>(event.start())};
    for (auto& evt: event.events()) {
        bool result = m_link.measurement("L1Event")
                <<Influx::Tag{"user", evt.data().user}
                <<Influx::Tag{"detector", evt.data().station_id}
                <<Influx::Tag{"site_id", evt.data().user + evt.data().station_id}
                <<Influx::Field{"accuracy", evt.data().time_acc}
                <<Influx::Field{"uuid", guid.to_string()}
                <<Influx::Field{"coinc_level", event.n()}
                <<Influx::Field{"counter", evt.data().ublox_counter}
                <<Influx::Field{"length", evt.duration()}
                <<Influx::Field{"coinc_time", evt.start() - event.start()}
                <<Influx::Field{"cluster_coinc_time", cluster_coinc_time}
                <<Influx::Field{"time_ref", evt.data().gnss_time_grid}
                <<Influx::Field{"valid_fix", evt.data().fix}
                <<evt.start();


        if (!result) {
            Log::error()<<"Could not write event to database.";
            return;
        }
    }
}


}
