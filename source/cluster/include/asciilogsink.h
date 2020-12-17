#ifndef ASCIILOGSINK_H
#define ASCIILOGSINK_H

#include "abstractsink.h"
#include "clusterlog.h"

#include <ostream>

namespace MuonPi {


/**
 * @brief The AsciiLogSink class. A Sink which writes ClusterLog messages to a std::ostream
 */
class AsciiLogSink : public AbstractSink<ClusterLog>
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
    [[nodiscard]] auto to_string(ClusterLog log) -> std::string;

    std::ostream& m_ostream;
};
}

#endif // ASCIILOGSINK_H
