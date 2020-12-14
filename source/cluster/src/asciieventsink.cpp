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
//	    m_ostream<<"Single Event:" + to_string(event) + "\n";
    }
}

auto AsciiEventSink::to_string(Event& evt) const -> std::string
{
    std::string output {};
    std::ostringstream out { output };
    std::chrono::seconds start { std::chrono::duration_cast<std::chrono::seconds>(evt.start().time_since_epoch()).count() };
    std::int64_t start_s { start.count() };
    std::int64_t offset_start { std::chrono::duration_cast<std::chrono::nanoseconds>(evt.start().time_since_epoch() - start).count() };
    std::int64_t offset_end { std::chrono::duration_cast<std::chrono::nanoseconds>(evt.end().time_since_epoch() - start).count() };
    out
            <<evt.hash()
            <<' '<<start_s
            <<' '<<offset_start
            <<' '<<offset_end;

    return out.str();
}
}
