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

auto ClusterLog::time() const -> std::chrono::system_clock::time_point
{
    return m_time;
}
	
	
}
