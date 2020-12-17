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
        double timeout;
        struct {
            double single_in;
            double l1_out;
        } frequency;

        std::size_t incoming;
        std::map<std::size_t, std::size_t> outgoing;
        std::size_t buffer_length;
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
