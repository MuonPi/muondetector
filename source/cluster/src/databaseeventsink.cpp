#include "databaseeventsink.h"

#include "event.h"
#include "databaselink.h"

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
	
	std::vector<Event> events = event.events();
	auto evt_it = events.begin();
	for (;evt_it != events.end(); evt_it++) {
		DbEntry entry { };
		// measurement name
		entry.measurement()="L1Event";
		// timestamp
		entry.timestamp()=std::to_string(evt_it->start());
		// tags
		entry.tags().push_back(std::make_pair("user", evt_it->data().user));
		entry.tags().push_back(std::make_pair("detector", evt_it->data().station_id));
		entry.tags().push_back(std::make_pair("site_id", evt_it->data().user+evt_it->data().station_id));
		// fields
		entry.fields().push_back(std::make_pair("accuracy", std::to_string(evt_it->data().time_acc)));
		if (m_link->write_entry(entry)) return;
	}
    // todo: construct message string
}
 
 
}
