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

void DatabaseEventSink::process(Event evt)
{
    DbEntry entry { "L1Event" };
    entry.timestamp() = std::to_string(evt.start());

    // todo: construct message string
}


}
