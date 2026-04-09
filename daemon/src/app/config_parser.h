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
    struct PresenceFlags {
        bool cliGpsDevice{false};
        bool cliGnssBaud{false};
        bool cliServerAddress{false};
        bool cliServerPort{false};
        bool cliTimingInput{false};
        bool cliBias{false};
        bool cliPreamp1{false};
        bool cliPreamp2{false};
        bool cliGain{false};
        bool cliTrigger{false};
        bool cliPolarity1{false};
        bool cliPolarity2{false};
        bool cliStationId{false};

        bool cfgGpsDevice{false};
        bool cfgGnssBaud{false};
        bool cfgServerAddress{false};
        bool cfgServerPort{false};
        bool cfgTimingInput{false};
        bool cfgBias{false};
        bool cfgPreamp1{false};
        bool cfgPreamp2{false};
        bool cfgGain{false};
        bool cfgTrigger{false};
        bool cfgPolarity1{false};
        bool cfgPolarity2{false};
        bool cfgStationId{false};
    };

    SystemConfig m_config;
    PresenceFlags m_presence;
    void print_help(const std::string& progName);
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
