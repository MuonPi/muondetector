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
    DbEntry entry { "Log" };
    // timestamp must not be ommited!
	unsigned long long nanosecondsUTC = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	entry.timestamp()=std::to_string(nanosecondsUTC);
	Log::debug()<<"DatabaseLogSink::process(ClusterLog log): cluster log timestamp = "<<std::to_string(nanosecondsUTC);
	// tags
    entry.tags().push_back(std::make_pair("user", "MuonPi"));

	// fields
    entry.fields().push_back(std::make_pair("timeout", static_cast<int>(log.data().timeout)));
    entry.fields().push_back(std::make_pair("frequency_in", std::make_pair(static_cast<double>(log.data().frequency.single_in),5)));
    entry.fields().push_back(std::make_pair("frequency_l1_out", std::make_pair(static_cast<double>(log.data().frequency.l1_out),5)));
    entry.fields().push_back(std::make_pair("buffer_length", static_cast<int>(log.data().buffer_length)));
    entry.fields().push_back(std::make_pair("total_detectors", static_cast<int>(log.data().total_detectors)));
    entry.fields().push_back(std::make_pair("reliable_detectors", static_cast<int>(log.data().reliable_detectors)));
    entry.fields().push_back(std::make_pair("max_multiplicity", static_cast<int>(log.data().maximum_n)));
    entry.fields().push_back(std::make_pair("incoming", static_cast<int>(log.data().incoming)));

    for (auto& [level, n]: log.data().outgoing) {
		if (level == 1) {
			continue;
		}
		entry.fields().push_back(std::make_pair("outgoing"+ std::to_string(level), static_cast<long int>(n)));
    }

	if (!m_link.write_entry(entry)) {
		Log::error()<<"Could not write event to database.";
		return;
	}
}


}
