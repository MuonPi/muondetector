#ifndef CONFIGURATION_H
#define CONFIGURATION_H


#include <string>
#include <map>

#include <libconfig.h++>

namespace MuonPi {


class Store
{
public:
    enum Type {
        Plain,
        Encrypted
    };

    template <typename T>
    struct Value
    {
        std::string key;
        T value;
    };

    Store(const std::string& name, Type t);

    template<typename T>
    [[nodiscard]] auto get(const std::string& key) -> T;

    template<typename T>
    void set(const Value<T>& value);

    template<typename T>
    auto operator>>(Value<T>& value) -> Store&;

    template<typename T>
    auto operator<(const Value<T>& value) -> Store&;

    [[nodiscard]] auto save() -> bool;

private:
    friend class Configuration;

    void load();
    void set_location(const std::string& location);

    Type m_type { Plain };
    std::string m_name {};
    std::string m_location {};
    libconfig::Config m_config;
};

template<typename T>
auto Store::get(const std::string& key) -> T
{
    return static_cast<T>(m_config.getRoot().lookup(key));
}

template<typename T>
void Store::set(const Value<T>& value)
{
    m_config.getRoot().lookup(value.key) = value.value;
}

template<typename T>
auto Store::operator>>(Value<T>& value) -> Store&
{
    value.value = static_cast<T>(m_config.getRoot().lookup(value.key));
    return *this;
}

template<typename T>
auto Store::operator<(const Value<T>& value) -> Store&
{
    m_config.getRoot().lookup(value.key) = value.value;
    return *this;
}

class Configuration
{

public:
    Configuration(const std::string& location);

    [[nodiscard]] auto operator[](const std::string& name) -> Store&;


private:
    std::string m_location {};
    std::map<std::string, Store> m_stores {};

};

}
#endif // CONFIGURATION_H
