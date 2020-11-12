#include "singleevent.h"

namespace MuonPi {

SingleEvent::~SingleEvent() = default;

auto SingleEvent::site_id() const -> std::string
{
    return m_site_id;
}

auto SingleEvent::username() const -> std::string
{
    return m_username;
}

}
