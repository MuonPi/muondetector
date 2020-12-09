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
{}

void MessageParser::skip_delimiter()
{
    while ((*m_iterator == m_delimiter) && (m_iterator != m_content.end())) {
        m_iterator++;
    }
}

auto MessageParser::consume_field() -> std::string
{
    std::string buffer {};
    skip_delimiter();
    while ((*m_iterator != m_delimiter) && (m_iterator != m_content.end())) {
        buffer += *m_iterator;
        m_iterator++;
    }
    return buffer;
}

auto MessageParser::has_field() -> bool
{
    return (m_iterator != m_content.end());
}

}
