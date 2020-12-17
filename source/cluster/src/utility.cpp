#include "utility.h"
#include "log.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <iomanip>
#include <fstream>
#include <regex>
#include <random>

namespace MuonPi {


MessageConstructor::MessageConstructor(char delimiter)
    : m_delimiter { delimiter }
{}

void MessageConstructor::add_field(const std::string& field)
{
    if (!m_message.empty()) {
        m_message += m_delimiter;
    }
    m_message += field;
}

auto MessageConstructor::get_string() const -> std::string
{
    return m_message;
}

MessageParser::MessageParser(const std::string& message, char delimiter)
    : m_content { message }
    , m_iterator { m_content.begin() }
    , m_delimiter { delimiter }
{
    while (!at_end()) {
        read_field();
    }
}

void MessageParser::skip_delimiter()
{
    while ((*m_iterator == m_delimiter) && (m_iterator != m_content.end())) {
        m_iterator++;
    }
}

void MessageParser::read_field()
{
    skip_delimiter();
    std::string::iterator start { m_iterator };
    while ((*m_iterator != m_delimiter) && (m_iterator != m_content.end())) {
        m_iterator++;
    }
    std::string::iterator end { m_iterator };

    if (start != end) {
        m_fields.push_back(std::make_pair(start, end));
    }
}

auto MessageParser::at_end() -> bool
{
    skip_delimiter();
    return m_iterator == m_content.end();
}

auto MessageParser::size() const -> std::size_t
{
    return m_fields.size();
}

auto MessageParser::empty() const -> bool
{
    return m_fields.empty();
}

auto MessageParser::operator[](std::size_t i) const -> std::string
{
    return std::string{ m_fields[i].first, m_fields[i].second };
}

GUID::GUID(std::size_t hash, std::uint64_t time)
    : m_first { get_mac() ^ hash ^ (get_number() & 0x00000000FFFFFFFF) }
    , m_second { time ^ (get_number() & 0xFFFFFFFF00000000) }
{}

auto GUID::to_string() const -> std::string
{
    std::string out_str {};
    std::ostringstream out { out_str };
    out<<std::right<<std::hex<<std::setfill('0')<<std::setw(16)<<m_first<<std::setw(16)<<m_second;
    return out.str();
}

auto GUID::get_mac() -> std::uint64_t
{
    static std::uint64_t addr { 0 };
    if (addr != 0) {
        return addr;
    }

    ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        Log::error()<<"Could not get MAC address.";
        return {};
    }

    std::string ifname {};
    for (ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }
        ifname = ifa->ifa_name;
        break;
    }
    freeifaddrs(ifaddr);
    if (ifname.empty()) {
        Log::error()<<"Could not get MAC address.";
        return {};
    }

    std::ifstream iface("/sys/class/net/" + ifname + "/address");
    std::string str((std::istreambuf_iterator<char>(iface)), std::istreambuf_iterator<char>());
    if (!str.empty()) {
        addr = std::stoull(std::regex_replace(str, std::regex(":"), ""), nullptr, 16);
    }
    return addr;
}

auto GUID::get_number() -> std::uint64_t
{
    static std::mt19937 gen { static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) };
    static std::uniform_int_distribution<std::uint64_t> distribution{0, std::numeric_limits<std::uint64_t>::max()};

    return distribution(gen);
}

}
