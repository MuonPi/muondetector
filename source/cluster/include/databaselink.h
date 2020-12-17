#ifndef DATABASELINK_H
#define DATABASELINK_H

#include "influxdb.hpp"

#include <string>
#include <vector>
#include <mutex>

namespace MuonPi {


class DbEntry {
public:
    DbEntry(const std::string& measurement);

    inline auto measurement() -> std::string& { return m_measurement; }
    inline auto measurement() const -> const std::string& { return m_measurement; }
    inline auto tags() -> std::vector<std::pair<std::string,std::string>>& { return m_tags; }
    inline auto tags() const -> const std::vector<std::pair<std::string,std::string>>& { return m_tags; }
    inline auto fields() -> std::vector<std::pair<std::string,std::string>>& { return m_fields; }
    inline auto fields() const -> const std::vector<std::pair<std::string,std::string>>& { return m_fields; }
    inline auto timestamp() -> std::string& { return m_time; }
    inline auto timestamp() const -> const std::string& { return m_time; }

private:
    std::string m_measurement {};
    std::vector<std::pair<std::string,std::string>> m_tags {};
    std::vector<std::pair<std::string,std::string>> m_fields {};
    std::string m_time {};
};

/**
 * @brief The DatabaseLink class
 */
class DatabaseLink
{
public:

    struct LoginData
    {
        std::string username {};
        std::string password {};
    };

    DatabaseLink(const std::string& server, const LoginData& login, const std::string& database);
    ~DatabaseLink();

    [[nodiscard]] auto write_entry(const DbEntry& entry) -> bool;

private:
    std::string m_server {};
    LoginData m_login_data {};
    std::string m_database {};
    influxdb_cpp::server_info m_server_info {"muonpi.org", 8086, "db", "", ""};
    std::mutex m_mutex;
};

}

#endif // DATABASELINK_H
