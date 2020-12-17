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
        std::string output { "Combined Event: (" + std::to_string(event.n()) + ")" };
        for (auto& evt: event.events()) {
            output += "\n\t" + to_string(evt);
        }

        m_ostream<<output + "\n"<<std::flush;
    } else {
//        std::string output { "Single Event: " };
//        for (auto& evt: event.events()) {
//            output += to_string(evt);
//        }
//        m_ostream<<output + "\n";
    }
}

auto AsciiEventSink::to_string(Event& evt) const -> std::string
{
    const Event::Data data { evt.data() };
    std::string output {};
    std::ostringstream out { output };

    out		<<std::hex
            <<evt.hash()
            <<' '<<data.user
            <<' '<<data.station_id
            <<' '<<data.epoch
            <<' '<<data.start
            <<' '<<data.end
            <<' '<<data.time_acc
            <<' '<<data.ublox_counter
            <<' '<<static_cast<std::uint16_t>(data.fix)
            <<' '<<static_cast<std::uint16_t>(data.utc)
            <<' '<<static_cast<std::uint16_t>(data.gnss_time_grid);
    return out.str();
}
}
