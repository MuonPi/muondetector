#ifndef ASCIILOGSINK_H
#define ASCIILOGSINK_H

#include "abstractsink.h"
#include "clusterlog.h"
#include "detectorsummary.h"

#include <ostream>
#include <sstream>

namespace MuonPi {


template <typename T>
/**
 * @brief The AsciiLogSink class. A Sink which writes ClusterLog messages to a std::ostream
 */
class AsciiLogSink : public AbstractSink<T>
{
public:
    /**
     * @brief AsciiLogSink
     * @param ostream The ostream to which the messages should be written
     */
    AsciiLogSink(std::ostream& ostream);

protected:
    /**
     * @brief step reimplemented from ThreadRunner
     * @return returns 0 when the event loop should continue running
     */
    [[nodiscard]] auto step() -> int override;

private:
    /**
     * @brief to_string Converts a ClusterLog object to string
     * @param log The ClusterLog object to convert
     * @return the string representation of the object
     */
    [[nodiscard]] auto to_string(T log) -> std::string;

    std::ostream& m_ostream;
};

template<typename T>
AsciiLogSink<T>::AsciiLogSink(std::ostream &ostream)
    : m_ostream { ostream }
{
    AbstractSink<T>::start();
}

template<typename T>
auto AsciiLogSink<T>::step() -> int
{
    if (AbstractSink<T>::has_items()) {
        m_ostream<<to_string(AbstractSink<T>::next_item()) + "\n"<<std::flush;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    return 0;
}

template<>
auto AsciiLogSink<ClusterLog>::to_string(ClusterLog log) -> std::string
{
    auto data { log.data() };
    std::string out_str;
    std::ostringstream out { out_str };

    out
            <<"Cluster Log:"
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

template<>
auto AsciiLogSink<DetectorSummary>::to_string(DetectorSummary log) -> std::string
{
    auto data { log.data() };
    std::string out_str;
    std::ostringstream out { out_str };

    out
            <<"Detector Log: "<<log.user_info().site_id()
            <<"\n\teventrate: "<<data.mean_eventrate
            <<"\n\tpulselength: "<<data.mean_pulselength
            <<"\n\tincoming: "<<data.incoming
            <<"\n\tublox counter progess: "<<data.ublox_counter_progress
            <<"\n\tdeadtime factor: "<<data.deadtime
            ;

    return out.str();
}

}

#endif // ASCIILOGSINK_H
