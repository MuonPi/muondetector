#include "containers/status.h"
#include <string>

status::status(const status_map &f_status_map)
    : m_status_map{std::move(f_status_map)} {}

auto status::byte_array() -> std::string
{
    return "";
}

auto status::json() -> std::string
{
    return "";
}