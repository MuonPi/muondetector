#include "asciieventsink.h"
#include "event.h"
#include "log.h"
#include "utility.h"
#include <iostream>
#include <sstream>

namespace MuonPi {

AsciiEventSink::AsciiEventSink(std::ostream& a_ostream)
    : AbstractSink<Event>{}
    , m_ostream { a_ostream }
{
    start();
}

AsciiEventSink::~AsciiEventSink() = default;

auto AsciiEventSink::step() -> int
{
    if (has_items()) {
        process(next_item());
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

void AsciiEventSink::process(Event event)
{
    if (event.n() > 1) {
        std::string output { "Combined Event:" };
        if (event.contested()) {
            output += " (contested)";
        }
        for (auto& evt: event.events()) {
            output += "\n\t" + to_string(evt);
        }

        m_ostream<<output + "\n";
    } else {
/*        std::string output { "Single Event: " };
        for (auto& evt: event.events()) {
            output += to_string(evt);
        }
        m_ostream<<output + "\n";*/
    }
}

auto AsciiEventSink::to_string(Event& evt) const -> std::string
{
    Event::Data data { evt.data() };
    std::string output {};
    std::ostringstream out { output };
    std::chrono::seconds start { std::chrono::duration_cast<std::chrono::seconds>(data.start.time_since_epoch()).count() };
    std::int64_t start_s { start.count() };
    std::int64_t offset_start { std::chrono::duration_cast<std::chrono::nanoseconds>(data.start.time_since_epoch() - start).count() };
    std::int64_t offset_end { std::chrono::duration_cast<std::chrono::nanoseconds>(data.end.time_since_epoch() - start).count() };

    out
            <<evt.hash()
            <<' '<<data.user
            <<' '<<data.station_id
            <<' '<<start_s
            <<' '<<offset_start
            <<' '<<offset_end
            <<' '<<data.time_acc
            <<' '<<data.ublox_counter
            <<' '<<data.fix
            <<' '<<data.utc
            <<' '<<data.gnss_time_grid;
    return out.str();
}
}
