#include "databaselink.h"

#include "log.h"

#include <type_traits>
#include <variant>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>


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
                   [this, field](const std::string& value){m_stream<<(m_has_field?',':' ')<<field.name<<"=\""<<value<<'"';},
                   [this, field](std::int_fast64_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](std::size_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](std::uint8_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](std::uint16_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](std::uint32_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](bool value){m_stream<<field.name<<(m_has_field?',':' ')<<'='<<value;},
                   [this, field](double value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;}
               }, field.value);
    return *this;
}

auto DatabaseLink::Entry::operator<<(std::int_fast64_t timestamp) -> bool
{
    m_stream<<' '<<timestamp;
    return m_link.send_string(m_stream.str());
}

DatabaseLink::DatabaseLink(const std::string& server, const LoginData& login, const std::string& database)
    : m_server { server }
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

    try {
      curlpp::Cleanup cleaner;
      curlpp::Easy request;


      std::ostringstream url {};
      url
            <<m_server
            <<" /write?db="
            <<m_database
            <<"&u="<<m_login_data.username
            <<"&p="<<m_login_data.password
            <<"&epoch=ms";

      request.setOpt(new curlpp::options::Url(url.str()));
      request.setOpt(new curlpp::options::Port(s_port));

      request.setOpt(new curlpp::options::PostFields(query));
      request.setOpt(new curlpp::options::PostFieldSize(static_cast<long>(query.length())));
      // +++ Debugging only
      request.setOpt(new curlpp::options::Verbose(true));
      request.setOpt((new curlpp::options::WriteStream(&std::cerr)));
      // --- Debugging only
      request.perform();
    }
    catch ( curlpp::LogicError & e ) {
      std::cout << e.what() << std::endl;
    }
    catch ( curlpp::RuntimeError & e ) {
      std::cout << e.what() << std::endl;
    }
    return {};
}

} // namespace MuonPi
