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
        if (m_link->write_entry(entry)) {
            return;
        }
    }
    // todo: construct message string
}


}
