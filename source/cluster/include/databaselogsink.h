#ifndef DATABASELOGSINK_H
#define DATABASELOGSINK_H

#include "abstractsink.h"

#include "databaselink.h"
#include "utility.h"
#include "log.h"
#include "clusterlog.h"
#include "detectorsummary.h"

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
    auto nanosecondsUTC { std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() };
    auto fields { std::move(m_link.measurement("cluster_summary")
            <<Influx::Tag{"cluster_id", m_link.cluster_id }
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

template <>
void DatabaseLogSink<DetectorSummary>::process(DetectorSummary log)
{
    auto nanosecondsUTC { std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() };
    auto result { std::move(m_link.measurement("detector_summary")
            <<Influx::Tag{"cluster_id", m_link.cluster_id }
            <<Influx::Tag{"user", log.user_info().username}
            <<Influx::Tag{"detector", log.user_info().station_id}
            <<Influx::Tag{"site_id", log.user_info().site_id()}
            <<Influx::Field{"eventrate", log.data().mean_eventrate}
            <<Influx::Field{"time_acc", log.data().mean_time_acc}
            <<Influx::Field{"pulselength", log.data().mean_pulselength}
            <<Influx::Field{"incoming", log.data().incoming}
            <<Influx::Field{"ublox_counter_progress", log.data().ublox_counter_progress}
            <<Influx::Field{"deadtime_factor", log.data().deadtime}
            <<nanosecondsUTC
            )};

    if (!result) {
        Log::error()<<"Could not write event to database.";
        return;
    }
}

} // namespace MuonPi
#endif // DATABASELOGSINK_H
