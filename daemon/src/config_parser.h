#ifndef COMMANDLINE_PARSER_H
#define COMMANDLINE_PARSER_H


#include "muondetector_structs.h"
#include <libconfig.h++>
#include <cstdint>
#include <string>
#include <memory>
#include <optional>


class ConfigParser
{
public:
    struct configuration {
        std::string username;
        std::string password;
        std::string gpsdevname { "" };
        int verbose { 0 };
        std::uint8_t pcaPortMask { 0 };
        std::array<float, 2> thresholdVoltage { -1.0F, -1.0F };
        float biasVoltage { -1.0F };
        bool bias_ON { false };
        GPIO_SIGNAL eventTrigger { EVT_XOR };
        std::string serverAddress { "" };
        std::uint16_t serverPort { 0 };
        bool showout { false };
        bool showin { false };
        std::array<bool, 2> preamp_enable { false, false };
        bool hi_gain { false };
        std::string station_ID { "0" };
        std::array<bool, 2> polarity { true, true };
        std::size_t maxGeohashLength { MuonPi::Settings::log.max_geohash_length };
        bool storeLocal { false };
        /* GNSS configs */
        bool gnss_dump_raw { false };
        int gnss_baudrate { 9600 };
        bool gnss_config { false };
        UbxDynamicModel gnss_dynamic_model { UbxDynamicModel::stationary };
        PositionModeConfig position_mode_config {
            PositionModeConfig::Mode::Auto,
            {},
            MuonPi::Config::max_lock_in_dop,
            MuonPi::Config::lock_in_target_precision_meters,
            PositionModeConfig::FilterType::None
        };
        std::shared_ptr<libconfig::Config> config_file_data {nullptr};
        std::shared_ptr<libconfig::Config> settings_file_data {nullptr};
    };

    ConfigParser(int argc, char* argv[], std::shared_ptr<configuration> f_config);
    ~ConfigParser();


    auto get() const -> const std::shared_ptr<configuration>;

private:
    std::shared_ptr<configuration> m_config;
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