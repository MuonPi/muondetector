#include "asciieventsink.h"
#include "event.h"
#include "combinedevent.h"
#include "log.h"
#include "utility.h"
#include <iostream>

namespace MuonPi {

AsciiEventSink::AsciiEventSink(std::ostream& a_ostream)
    : m_ostream { a_ostream }
{

}

AsciiEventSink::~AsciiEventSink() = default;

auto AsciiEventSink::step() -> int
{
    if (has_items()) {
        std::unique_ptr<Event> e { next_item()};
        if (e->n() > 1) {
            std::unique_ptr<CombinedEvent> evt_ptr {dynamic_cast<CombinedEvent*>(e.get())};
            process(std::move(evt_ptr));
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

void AsciiEventSink::process(std::unique_ptr<CombinedEvent> /*evt*/)
{
    // todo: stream the message into m_ostream
}

}
