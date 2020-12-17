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
    for (auto& evt: event.events()) {
        DbEntry entry { "L1Event" };
        // timestamp
        entry.timestamp()=std::to_string(static_cast<std::uint64_t>(evt.epoch())*static_cast<std::uint64_t>(1e9) + static_cast<std::uint64_t>(evt.start()));
        // tags
        entry.tags().push_back(std::make_pair("user", evt.data().user));
        entry.tags().push_back(std::make_pair("detector", evt.data().station_id));
        entry.tags().push_back(std::make_pair("site_id", evt.data().user+evt.data().station_id));
        // fields
        entry.fields().push_back(std::make_pair("accuracy", std::to_string(evt.data().time_acc)));

        GUID guid{evt.hash(), static_cast<std::uint64_t>(evt.epoch()) * 1000000000 + static_cast<std::uint64_t>(evt.start())};
        entry.fields().push_back(std::make_pair("uuid", guid.to_string()));

        entry.fields().push_back(std::make_pair("coinc_level", std::to_string(event.n())));
        entry.fields().push_back(std::make_pair("counter", std::to_string(evt.data().ublox_counter)));
        entry.fields().push_back(std::make_pair("length", std::to_string(evt.duration())));

        /*
         * TODO: Implement calculation of time differences to first event (evt_coinc_time)
         * and storage of the coincidence span (diff btw. first to last ts)
         */

        const std::int64_t evt_coinc_time = (evt.epoch() - event.epoch()) * static_cast<std::int64_t>(1e9) + (evt.start() - event.start());

        entry.fields().push_back(std::make_pair("coinc_time", std::to_string(evt_coinc_time)));
        entry.fields().push_back(std::make_pair("cluster_coinc_time", std::to_string(cluster_coinc_time)));
        entry.fields().push_back(std::make_pair("time_ref", std::to_string(evt.data().gnss_time_grid)));
        entry.fields().push_back(std::make_pair("valid_fix", std::to_string(evt.data().fix)));

        if (!m_link.write_entry(entry)) {
            Log::error()<<"Could not write event to database.";
            return;
        }
    }
}


}
