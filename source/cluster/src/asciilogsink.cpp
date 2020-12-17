#include "asciilogsink.h"

#include <chrono>
#include <sstream>

namespace MuonPi {

AsciiLogSink::AsciiLogSink(std::ostream &ostream)
    : m_ostream { ostream }
{
    start();
}

auto AsciiLogSink::step() -> int
{
    if (has_items()) {
        m_ostream<<to_string(next_item()) + "\n"<<std::flush;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    return 0;
}

auto AsciiLogSink::to_string(ClusterLog log) -> std::string
{
    auto data { log.data() };
    std::string out_str;
    std::ostringstream out { out_str };

    out
            << "Log: "
            <<" timeout: "<<data.timeout<<" ms"
            <<" in: "<<data.frequency.single_in<<" Hz"
            <<" out: "<<data.frequency.l1_out<<" Hz"
            <<" buffer: "<<data.buffer_length
            <<" events in interval: "<<data.incoming
            <<" out in interval: ";
    for (auto& [n, i]: data.outgoing) {
        out<<"("<<n<<":"<<i<<") ";
    }

    return out.str();
}
}
