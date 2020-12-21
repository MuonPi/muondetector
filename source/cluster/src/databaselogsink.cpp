#include "databaselogsink.h"

#include "databaselink.h"
#include "utility.h"
#include "log.h"
#include "clusterlog.h"

#include <sstream>

namespace MuonPi {

DatabaseLogSink::DatabaseLogSink(DatabaseLink& link)
    : m_link { link }
{
    start();
}

auto DatabaseLogSink::step() -> int
{
    if (has_items()) {
        process(next_item());
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

void DatabaseLogSink::process(ClusterLog log)
{
    auto nanosecondsUTC { std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() };
    auto fields { std::move(m_link.measurement("Log")
            <<Influx::Tag{"user", "MuonPi"}
            <<Influx::Field{"timeout", log.data().timeout}
            <<Influx::Field{"frequency_in", log.data().frequency.single_in}
            <<Influx::Field{"frequency_l1_out", log.data().frequency.l1_out}
            <<Influx::Field{"buffer_length", log.data().buffer_length}
            <<Influx::Field{"total_detectors", log.data().total_detectors}
            <<Influx::Field{"reliable_detectors", log.data().reliable_detectors}
            <<Influx::Field{"max_multiplicity", log.data().maximum_n}
            <<Influx::Field{"incoming", log.data().incoming}
            )};

    for (auto& [level, n]: log.data().outgoing) {
        if (level == 1) {
            continue;
        }
        fields<<Influx::Field{"outgoing"+ std::to_string(level), n};
    }
    auto result { fields<<nanosecondsUTC };

    if (!result) {
        Log::error()<<"Could not write event to database.";
        return;
    }
}


}
