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
            << "Log:"
            <<"\n\ttimeout: "<<data.timeout<<" ms"
            <<"\n\tin: "<<data.frequency.single_in<<" Hz"
            <<"\n\tout: "<<data.frequency.l1_out<<" Hz"
            <<"\n\tbuffer: "<<data.buffer_length
            <<"\n\tevents in interval: "<<data.incoming
            <<"\n\tout in interval: ";
    for (auto& [n, i]: data.outgoing) {
        out<<"("<<n<<":"<<i<<") ";
    }
    out
            <<"\n\tdetectors: "<<data.total_detectors<<"("<<data.reliable_detectors<<")"
            <<"\n\tmaximum n: "<<data.maximum_n;

    return out.str();
}
}
