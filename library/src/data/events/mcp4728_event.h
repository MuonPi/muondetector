#ifndef MCP4728_EVENT_H
#define MCP4728_EVENT_H

#include <cstdint>
#include <unordered_map>

struct MCP4728Event
{
    std::unordered_map<std::uint8_t, std::uint16_t> data;
};

#endif // MCP4728_EVENT_H