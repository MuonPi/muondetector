#include "databaselink.h"

#include "log.h"

namespace MuonPi {
DbEntry::DbEntry(const std::string& measurement)
    : m_measurement { measurement }
{}

DatabaseLink::DatabaseLink(const std::string& server, const LoginData& login, const std::string& database)
    : m_server { server }
    , m_login_data { login }
    , m_database { database }
    , m_server_info { influxdb_cpp::server_info(m_server, 8086, m_database, m_login_data.username, m_login_data.password) }
{

}

DatabaseLink::~DatabaseLink() = default;


auto DatabaseLink::write_entry(const DbEntry& entry) -> bool
{
    influxdb_cpp::builder builder{};
    influxdb_cpp::detail::tag_caller& tags { builder.meas(entry.measurement()) };

    if (!entry.tags().empty()) {
        for (auto& [key, value] : entry.tags()) {
            tags.tag(key, value);
        }
    }

    if (entry.fields().empty()) {
        return false;
    }
    std::vector<std::pair<std::string, std::variant<std::string,bool,short,int,long,long long,std::pair<double, int>>>> field_list { entry.fields() };

    influxdb_cpp::detail::field_caller& fields { add_field(field_list[0].first, field_list[0].second, tags) };

    field_list.erase(field_list.begin());

    for (auto& [key, value] : field_list) {
        add_field(key, value, fields);
    }

    std::scoped_lock<std::mutex> lock { m_mutex };

    int result = fields.timestamp(stoull(entry.timestamp(), nullptr))
            .post_http(m_server_info);

    if (result == -3) {
        Log::warning()<<"Database not connected.";
    } else if (result != 0) {
        Log::warning()<<"Unspecified database error: " + std::to_string(result);
    }

    return result == 0;
}

auto DatabaseLink::add_field(const std::string& name, const std::variant<std::string,bool,short,int,long,long long,std::pair<double, int>>& value, influxdb_cpp::detail::field_caller& caller) -> influxdb_cpp::detail::field_caller&
{
    if (std::holds_alternative<std::string>(value)) {
        return caller.field(name, std::get<std::string>(value));
    } else if (std::holds_alternative<bool>(value)) {
        return caller.field(name, std::get<bool>(value));
    } else if (std::holds_alternative<short>(value)) {
        return caller.field(name, std::get<short>(value));
    } else if (std::holds_alternative<int>(value)) {
        return caller.field(name, std::get<int>(value));
    } else if (std::holds_alternative<long>(value)) {
        return caller.field(name, std::get<long>(value));
    } else if (std::holds_alternative<long long>(value)) {
        return caller.field(name, std::get<long long>(value));
    } else {
        auto pair { std::get<std::pair<double, int>>(value) };
        return caller.field(name, pair.first, pair.second);
    }
}

auto DatabaseLink::add_field(const std::string& name, const std::variant<std::string,bool,short,int,long,long long,std::pair<double, int>>& value, influxdb_cpp::detail::tag_caller& caller) -> influxdb_cpp::detail::field_caller&
{
    if (std::holds_alternative<std::string>(value)) {
        return caller.field(name, std::get<std::string>(value));
    } else if (std::holds_alternative<bool>(value)) {
        return caller.field(name, std::get<bool>(value));
    } else if (std::holds_alternative<short>(value)) {
        return caller.field(name, std::get<short>(value));
    } else if (std::holds_alternative<int>(value)) {
        return caller.field(name, std::get<int>(value));
    } else if (std::holds_alternative<long>(value)) {
        return caller.field(name, std::get<long>(value));
    } else if (std::holds_alternative<long long>(value)) {
        return caller.field(name, std::get<long long>(value));
    } else {
        auto pair { std::get<std::pair<double, int>>(value) };
        return caller.field(name, pair.first, pair.second);
    }
}

} // namespace MuonPi
