#include "databaseeventsink.h"

#include "event.h"
#include "databaselink.h"
#include <sstream>

namespace MuonPi {

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

    for (auto& evt: event.events()) {
        DbEntry entry { "L1Event" };
        // timestamp
        entry.timestamp()=std::to_string(evt.start());
        // tags
        entry.tags().push_back(std::make_pair("user", evt.data().user));
        entry.tags().push_back(std::make_pair("detector", evt.data().station_id));
        entry.tags().push_back(std::make_pair("site_id", evt.data().user+evt.data().station_id));
		// fields
        entry.fields().push_back(std::make_pair("accuracy", std::to_string(evt.data().time_acc)));
        std::stringstream sstream;
		sstream << std::hex << event.id();
		std::string id_str = sstream.str();
		entry.fields().push_back(std::make_pair("uuid", id_str));
        entry.fields().push_back(std::make_pair("coinc_level", std::to_string((int)event.n())));
        entry.fields().push_back(std::make_pair("counter", std::to_string(evt.data().ublox_counter)));
        entry.fields().push_back(std::make_pair("length", std::to_string(evt.duration())));
        
		/*
		 * TODO: Implement calculation of time differences to first event (evt_coinc_time)
		 * and storage of the coincidence span (diff btw. first to last ts)
		 */
		
		std::uint64_t evt_coinc_time = 0;
        std::uint64_t cluster_coinc_time = 0;
		entry.fields().push_back(std::make_pair("coinc_time", std::to_string(evt_coinc_time)));
		entry.fields().push_back(std::make_pair("cluster_coinc_time", std::to_string(cluster_coinc_time)));
        entry.fields().push_back(std::make_pair("time_ref", std::to_string((int)evt.data().gnss_time_grid)));
        entry.fields().push_back(std::make_pair("valid_fix", std::to_string((int)evt.data().fix)));
		
        if (m_link->write_entry(entry)) {
            return;
        }
    }
}


}
