#include "configuration.h"

namespace MuonPi {

Configuration::Configuration(const std::string& location)
    : m_location { location }
{

}
auto Configuration::operator[](const std::string& name) -> Store&
{
    return m_stores[name];
}

auto Store::save() -> bool
{
    if (m_type == Plain) {

    }
}

void Store::load()
{

}

}
