#ifndef CLUSTERLOG_H
#define CLUSTERLOG_H

#include <chrono>
#include <string>
#include <map>

namespace MuonPi {


/**
 * @brief The ClusterLog class
 */
class ClusterLog
{
public:
    struct Data {
        double timeout { 0 };
        struct {
            double single_in { 0 };
            double l1_out { 0 };
        } frequency;

        std::size_t incoming { 0 };
        std::map<std::size_t, std::size_t> outgoing {};
        std::size_t buffer_length { 0 };
        std::size_t total_detectors { 0 };
        std::size_t reliable_detectors { 0 };
    };

    ClusterLog(Data data);
    ClusterLog();

    [[nodiscard]] auto data() -> Data;

private:
    Data m_data;
    bool m_valid { true };
};
}

#endif // CLUSTERLOG_H
