#include "utility.h"

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

}
