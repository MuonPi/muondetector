#ifndef UTILITY_H
#define UTILITY_H

#include <string>

namespace MuonPi {

class MessageConstructor
{
public:
    MessageConstructor(char delimiter);

    void add_field(const std::string& field);

    [[nodiscard]] auto get_string() const -> std::string;

private:
    std::string m_message {};
    char m_delimiter;
};


class MessageParser
{
public:
    MessageParser(const std::string& message, char delimiter);

    [[nodiscard]] auto consume_field() -> std::string;
    [[nodiscard]] auto has_field() -> bool;

private:
    void skip_delimiter();

    std::string m_content {};
    std::string::iterator m_iterator;
    char m_delimiter {};
};

}
#endif // UTILITY_H
