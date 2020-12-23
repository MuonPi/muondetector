#ifndef DATABASELINK_H
#define DATABASELINK_H

#include <string>
#include <vector>
#include <variant>
#include <mutex>
#include <sstream>

namespace MuonPi {


namespace Influx {

struct Tag {
    std::string name;
    std::string field;
};
struct Field {
    std::string name;
    std::variant<std::string,bool,std::int_fast64_t,double,std::size_t,std::uint8_t,std::uint16_t,std::uint32_t> value;
};
}

/**
 * @brief The DatabaseLink class
 */
class DatabaseLink
{
public:
    class Entry {
    public:
        Entry() = delete;

        auto operator<<(const Influx::Tag& tag) -> Entry&;
        auto operator<<(const Influx::Field& tag) -> Entry&;
        [[nodiscard]] auto operator<<(std::int_fast64_t timestamp) -> bool;

    private:
        std::ostringstream m_stream {};
        bool m_has_field { false };
        DatabaseLink& m_link;

        friend class DatabaseLink;

        Entry(const std::string& measurement, DatabaseLink& link);
    };

    struct LoginData
    {
        std::string username {};
        std::string password {};
    };

	std::string cluster_id { "muonpi.org" };
	
    DatabaseLink(const std::string& server, const LoginData& login, const std::string& database, const std::string& a_cluster_id = "muonpi.org");
    ~DatabaseLink();

    [[nodiscard]] auto measurement(const std::string& measurement) -> Entry;

private:
    [[nodiscard]] auto send_string(const std::string& query) -> bool;

    static constexpr short s_port { 8086 };

    std::string m_server {};
    LoginData m_login_data {};
    std::string m_database {};
    std::mutex m_mutex;
};

}

#endif // DATABASELINK_H
