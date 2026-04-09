#ifndef COMMANDLINE_PARSER_H
#define COMMANDLINE_PARSER_H

#include "system_config.h"

#include <libconfig.h++>
#include <cstdint>
#include <string>
#include <memory>
#include <optional>


class ConfigParser
{
public:

    ConfigParser(int argc, char* argv[], SystemConfig && f_config);
    ~ConfigParser();


    auto get() const -> SystemConfig;

private:
    SystemConfig m_config;
    void parse(int argc, char* argv[]);
    void apply_defaults();
    void validate();
    void report();

    // helpers
    auto static is_flag(const std::string& arg) -> bool;
    auto static strip_prefix(const std::string& arg) -> std::string;
    auto is_valid_ipv4(const std::string& ip) -> bool;
};
#endif // COMMANDLINE_PARSER_H