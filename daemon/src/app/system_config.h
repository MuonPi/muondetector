#ifndef SYSTEM_CONFIG
#define SYSTEM_CONFIG

#include "muondetector_structs.h"

#include <libconfig.h++>
#include <memory>
#include <string>

struct SystemConfig {
    std::string username;
    std::string password;
    std::string gpsdevname{""};
    std::string logLevel{"Warning"};
    std::uint8_t pcaPortMask{0};
    std::array<float, 2> thresholdVoltage{-1.0F, -1.0F};
    float biasVoltage{-1.0F};
    bool bias_ON{false};
    GPIO_SIGNAL eventTrigger{EVT_XOR};
    std::string serverAddress{"0.0.0.0"};
    std::uint16_t serverPort{51508};
    bool showout{false};
    bool showin{false};
    std::array<bool, 2> preamp_enable{false, false};
    bool hi_gain{false};
    std::string station_ID{"0"};
    std::array<bool, 2> polarity{true, true};
    std::size_t maxGeohashLength{MuonPi::Settings::log.max_geohash_length};
    bool storeLocal{false};
    /* GNSS configs */
    bool gnss_dump_raw{false};
    int gnss_baudrate{9600};
    bool gnss_config{false};
    UbxDynamicModel gnss_dynamic_model{UbxDynamicModel::stationary};
    PositionModeConfig position_mode_config{PositionModeConfig::Mode::Auto,
                                            {},
                                            MuonPi::Config::max_lock_in_dop,
                                            MuonPi::Config::lock_in_target_precision_meters,
                                            PositionModeConfig::FilterType::None};
    std::shared_ptr<libconfig::Config> config_file_data{nullptr};
    std::shared_ptr<libconfig::Config> settings_file_data{nullptr};
    std::string hardwareConfigPath{"/etc/muondetector/hardware.conf"};
    std::string componentConfigPath{"/etc/muondetector/components.conf"};
    std::size_t max_thread_count{0};
};

#endif // SYSTEM_CONFIG