#include "clusterlog.h"

#include <utility>

namespace MuonPi {

ClusterLog::ClusterLog(Data data)
    : m_data { data }
{}

ClusterLog::ClusterLog()
    : m_valid { false }
{}


auto ClusterLog::data() -> Data
{
    return m_data;
}
}
