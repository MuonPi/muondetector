#ifndef DATABASELOGSINK_H
#define DATABASELOGSINK_H

#include "abstractsink.h"

#include "databaselink.h"
#include "utility.h"
#include "log.h"
#include "clusterlog.h"
#include "detectorlog.h"

#include <sstream>
#include <memory>

namespace MuonPi {

template <class T>
/**
 * @brief The DatabaseLogSink class
 */
class DatabaseLogSink : public AbstractSink<T>
{
public:
	/**
     * @brief DatabaseLogSink
	 * @param link a DatabaseLink instance
     */
    DatabaseLogSink(DatabaseLink& link);

protected:
	/**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
	void process(T log);
	DatabaseLink& m_link;
};

// +++++++++++++++++++++++++++++++
// implementation part starts here
// +++++++++++++++++++++++++++++++

template <class T>
DatabaseLogSink<T>::DatabaseLogSink(DatabaseLink& link)
    : m_link { link }
{
    AbstractSink<T>::start();
}

template <class T>
auto DatabaseLogSink<T>::step() -> int
{
    if (AbstractSink<T>::has_items()) {
        process(AbstractSink<T>::next_item());
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

template <>
void DatabaseLogSink<ClusterLog>::process(ClusterLog log)
{
    DbEntry entry { "Log" };
    // timestamp must not be ommited!
	unsigned long long nanosecondsUTC = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	entry.timestamp()=std::to_string(nanosecondsUTC);
	Log::debug()<<"DatabaseLogSink::process(ClusterLog log): cluster log timestamp="<<std::to_string(nanosecondsUTC);
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

template <>
void DatabaseLogSink<DetectorLog>::process(DetectorLog /*log*/)
{
    DbEntry entry { "Log" };
    // timestamp must not be ommited!
	unsigned long long nanosecondsUTC = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	entry.timestamp()=std::to_string(nanosecondsUTC);
	Log::debug()<<"DatabaseLogSink::process(DetectorLog log): cluster log timestamp="<<std::to_string(nanosecondsUTC);
	// tags
    entry.tags().push_back(std::make_pair("user", "MuonPi"));

	// fields
/*
	entry.fields().push_back(std::make_pair("timeout", static_cast<int>(log.data().timeout)));
    entry.fields().push_back(std::make_pair("frequency_in", std::make_pair(static_cast<double>(log.data().frequency.single_in),5)));
    entry.fields().push_back(std::make_pair("frequency_l1_out", std::make_pair(static_cast<double>(log.data().frequency.l1_out),5)));
    entry.fields().push_back(std::make_pair("buffer_length", static_cast<int>(log.data().buffer_length)));
    entry.fields().push_back(std::make_pair("total_detectors", static_cast<int>(log.data().total_detectors)));
    entry.fields().push_back(std::make_pair("reliable_detectors", static_cast<int>(log.data().reliable_detectors)));
    entry.fields().push_back(std::make_pair("max_multiplicity", static_cast<int>(log.data().maximum_n)));
    entry.fields().push_back(std::make_pair("incoming", static_cast<int>(log.data().incoming)));
*/
	if (!m_link.write_entry(entry)) {
		Log::error()<<"Could not write event to database.";
		return;
	}
}








} // namespace MuonPi
#endif // DATABASELOGSINK_H
