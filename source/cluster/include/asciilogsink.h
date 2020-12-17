#ifndef ASCIILOGSINK_H
#define ASCIILOGSINK_H

#include "abstractsink.h"
#include "clusterlog.h"

#include <ostream>

namespace MuonPi {


class AsciiLogSink : public AbstractSink<ClusterLog>
{
public:
    AsciiLogSink(std::ostream& ostream);

protected:
    [[nodiscard]] auto step() -> int override;

private:
    [[nodiscard]] auto to_string(ClusterLog log) -> std::string;

    std::ostream& m_ostream;
};
}

#endif // ASCIILOGSINK_H
