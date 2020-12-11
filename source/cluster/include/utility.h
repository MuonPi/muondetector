#ifndef UTILITY_H
#define UTILITY_H

#include <string>
#include <vector>
#include <string_view>

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

    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;

    [[nodiscard]] auto operator[](std::size_t i) const -> std::string;

private:
    void skip_delimiter();
    void read_field();
    [[nodiscard]] auto at_end() -> bool;

    std::string m_content {};
    std::string::iterator m_iterator;
    char m_delimiter {};

    std::vector<std::pair<std::string::iterator, std::string::iterator>> m_fields {};
};

}
#endif // UTILITY_H
