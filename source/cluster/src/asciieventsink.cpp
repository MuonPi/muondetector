#include "asciieventsink.h"
#include "event.h"
#include "combinedevent.h"
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
        std::unique_ptr<Event> e { next_item() };
        if (e->n() > 1) {
            std::unique_ptr<CombinedEvent> evt_ptr {dynamic_cast<CombinedEvent*>(e.get())};
            process(std::move(evt_ptr));
            e.release();
        } else {
            process(std::move(e));
        }
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

void AsciiEventSink::process(std::unique_ptr<Event> /*evt*/)
{
    // todo: stream the message into m_ostream
}

void AsciiEventSink::process(std::unique_ptr<CombinedEvent> evt)
{
    m_ostream
            <<"Combined Event:\n";
    for (auto& event: evt->events()) {
        m_ostream<<' '<<to_string(*event);
    }
    // todo: stream the message into m_ostream
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
