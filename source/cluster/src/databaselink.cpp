#include "databaselink.h"

#include "log.h"

#include <type_traits>
#include <variant>

#include <curl/curl.h>

namespace MuonPi {
DatabaseLink::Entry::Entry(const std::string& measurement, DatabaseLink& link)
    : m_link { link }
{
    m_stream<<measurement;
}

auto DatabaseLink::Entry::operator<<(const Influx::Tag& tag) -> Entry&
{
    m_stream<<','<<tag.name<<'='<<tag.field;
    return *this;
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

auto DatabaseLink::Entry::operator<<(const Influx::Field& field) -> Entry&
{
    std::visit(overloaded {
                   [this, field](const std::string& value){m_stream<<(m_has_field?',':' ')<<field.name<<"=\""<<value<<'"'; m_has_field = true;},
                   [this, field](std::int_fast64_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value<<'i'; m_has_field = true;},
                   [this, field](std::size_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value<<'i'; m_has_field = true;},
                   [this, field](std::uint8_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<static_cast<std::uint16_t>(value)<<'i'; m_has_field = true;},
                   [this, field](std::uint16_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value<<'i'; m_has_field = true;},
                   [this, field](std::uint32_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value<<'i'; m_has_field = true;},
                   [this, field](bool value){m_stream<<field.name<<(m_has_field?',':' ')<<'='<<(value?'t':'f'); m_has_field = true;},
                   [this, field](double value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value; m_has_field = true;}
               }, field.value);
    return *this;
}

auto DatabaseLink::Entry::operator<<(std::int_fast64_t timestamp) -> bool
{
    m_stream<<' '<<timestamp;
    return m_link.send_string(m_stream.str());
}

DatabaseLink::DatabaseLink(const std::string& server, const LoginData& login, const std::string& database, const std::string& a_cluster_id)
    : cluster_id { a_cluster_id }
    , m_server { server }
    , m_login_data { login }
    , m_database { database }
    
{

}

DatabaseLink::~DatabaseLink() = default;

auto DatabaseLink::measurement(const std::string& measurement) -> Entry
{
    return Entry{measurement, *this};
}


auto DatabaseLink::send_string(const std::string& query) -> bool
{
    CURL *curl { curl_easy_init() };

    class CurlGuard
    {
    public:
        CurlGuard(CURL* curl) : m_curl { curl } {}
        ~CurlGuard() { if (m_curl != nullptr) curl_easy_cleanup(m_curl);}
    private:
        CURL* m_curl { nullptr };
    } curl_guard{curl};

    if(curl) {
        std::ostringstream url {};
        url
            <<m_server
            <<"/write?db="
            <<m_database
            <<"&u="<<m_login_data.username
            <<"&p="<<m_login_data.password
            <<"&epoch=ms";

        curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, s_port);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);


        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());

        CURLcode res { curl_easy_perform(curl) };

        long http_code { 0 };
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (res != CURLE_OK) {
            Log::warning()<<"Couldn't write to Database: " + std::to_string(http_code) + ": " + std::string{curl_easy_strerror(res)};
            return false;
        } else if ((http_code / 100) != 2) {
            Log::warning()<<"Couldn't write to Database: " + std::to_string(http_code);
            return false;
        }
    }
    return true;
}

} // namespace MuonPi
