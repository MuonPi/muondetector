#include "config_parser.h"

#include "core/logging/logger.h"
#include "system_config.h"

#include <algorithm>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif
#include <cstring>
#include <filesystem>
#include <libconfig.h++>
#include <limits>
#include <memory>
#include <vector>

ConfigParser::ConfigParser(int argc, char* argv[], SystemConfig&& f_config) : m_config{f_config} {
    apply_defaults();
    parse(argc, argv);
    setLogLevel(m_config.logLevel);
    logInfo("Loglevel is " + m_config.logLevel);
    validate();
    report();
}

ConfigParser::~ConfigParser() {
}

auto ConfigParser::loadConfigFile(const std::string& file) -> std::shared_ptr<libconfig::Config> {
    auto cfg = std::make_shared<libconfig::Config>();
    try {
        cfg->readFile(file.c_str());
    } catch (const libconfig::FileIOException& fioex) {
        logError("Error while reading config file " + file);
    } catch (const libconfig::ParseException& pex) {
        logError("Parse error at " + std::string(pex.getFile()) + " : line " +
                 std::to_string(pex.getLine()) + " - " + std::string(pex.getError()));
        throw pex;
    }
    return cfg;
}

auto ConfigParser::get() const -> SystemConfig {
    return m_config;
}

bool ConfigParser::is_flag(const std::string& arg) {
    return arg.rfind("-", 0) == 0;
}

std::string ConfigParser::strip_prefix(const std::string& arg) {
    if (arg.rfind("--", 0) == 0)
        return arg.substr(2);
    if (arg.rfind("-", 0) == 0)
        return arg.substr(1);
    return arg;
}

void ConfigParser::print_help(const std::string& progName) {
    std::cout << "Usage: " << progName << " [options] [gps_device]\n\n";

    std::cout << "General:\n";
    std::cout << "  -l, --logging <level>     Set verbosity level\n";
    std::cout << "  -h, --help                Show this help message\n\n";

    std::cout << "GNSS:\n";
    std::cout << "  -b <baud>                 Set GNSS baudrate\n";
    std::cout << "  -d                        Dump raw GNSS data\n";
    std::cout << "  --showin, --showincoming  Show incoming UBX messages as hex\n";
    std::cout << "  --showout, --showoutgoing Show outgoing UBX messages as hex\n";
    std::cout << "  [gps_device]              Device path (e.g. /dev/ttyUSB0)\n\n";

    std::cout << "Networking:\n";
    std::cout << "  --server <ip>             Set server address\n";
    std::cout << "  --dp <port>               Set server port\n\n";

    std::cout << "DAQ Settings:\n";
    std::cout << "  --th1 <V>                 Threshold channel 1\n";
    std::cout << "  --th2 <V>                 Threshold channel 2\n";
    std::cout << "  --bias <V>                Bias voltage\n";
    std::cout << "  -p                        Enable bias\n\n";

    std::cout << "Hardware:\n";
    std::cout << "  --pre1                    Enable preamp 1\n";
    std::cout << "  --pre2                    Enable preamp 2\n";
    std::cout << "  -g, --gain                Enable high gain\n";
    std::cout << "  --pol1 <0|1>              Set polarity channel 1\n";
    std::cout << "  --pol2 <0|1>              Set polarity channel 2\n\n";

    std::cout << "Trigger:\n";
    std::cout << "  -t, --trigger <mode>\n";
    std::cout << "       0 = XOR\n";
    std::cout << "       1 = AND\n";
    std::cout << "       2 = TIME_MEAS_OUT\n";
    std::cout << "       3 = EXT_TRIGGER\n\n";

    std::cout << "Misc:\n";
    std::cout << "  --id <string>             Station ID\n";
    std::cout << "  --pca <mask>              PCA port mask\n";
    std::cout << "  --sds011_n_sleep <n>      SDS011 sleep/readout interval parameter\n";
}

void ConfigParser::parse(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // -------- positional (device) --------
        if (!is_flag(arg)) {
            if (m_config.gpsdevname.empty()) {
                m_config.gpsdevname = arg;
                m_presence.cliGpsDevice = true;
            }
            continue;
        }

        std::string key = strip_prefix(arg);

        auto next_value = [&](std::string& out) -> bool {
            if (i + 1 < argc && !is_flag(argv[i + 1])) {
                out = argv[++i];
                return true;
            }
            return false;
        };

        std::string value;

        // -------- verbosity --------
        if (key == "l" || key == "logging") {
            if (next_value(value)) {
                std::vector<std::string> allowed({"Warning", "Error", "Info", "Debug"});
                if (std::find(allowed.begin(), allowed.end(), value) != allowed.end()) {
                    m_config.logLevel = value;
                } else {
                    logWarn("Value " + value + " invalid for argument -l --logging");
                }
            }
        }

        // -------- dump raw --------
        else if (key == "d") {
            m_config.gnss_dump_raw = true;
        }

        // -------- show in/out --------
        else if (key == "showin" || key == "showincoming") {
            m_config.showin = true;
        } else if (key == "showout" || key == "showoutgoing") {
            m_config.showout = true;
        }

        // -------- baudrate --------
        else if (key == "b") {
            if (next_value(value)) {
                try {
                    m_config.gnss_baudrate = std::stoi(value);
                    m_presence.cliGnssBaud = true;
                } catch (...) {
                }
            }
        }

        // -------- server --------
        else if (key == "server" || key == "daemonAddress") {
            if (next_value(value)) {
                m_config.serverAddress = value;
                m_presence.cliServerAddress = true;
            }
        }

        else if (key == "dp" || key == "daemonPort") {
            if (next_value(value)) {
                try {
                    m_config.serverPort = static_cast<uint16_t>(std::stoul(value));
                    m_presence.cliServerPort = true;
                } catch (...) {
                }
            }
        }

        // -------- thresholds --------
        else if (key == "th1" || key == "discr1") {
            if (next_value(value)) {
                try {
                    m_config.thresholdVoltage[0] = std::stof(value);
                } catch (...) {
                }
            }
        }

        else if (key == "th2" || key == "discr2") {
            if (next_value(value)) {
                try {
                    m_config.thresholdVoltage[1] = std::stof(value);
                } catch (...) {
                }
            }
        }

        // -------- bias --------
        else if (key == "bias") {
            if (next_value(value)) {
                try {
                    m_config.biasVoltage = std::stof(value);
                    m_presence.cliBias = true;
                } catch (...) {
                }
            }
        }

        else if (key == "p") {
            m_config.bias_ON = true;
            m_presence.cliBias = true;
        }

        // -------- preamps --------
        else if (key == "pre1") {
            m_config.preamp_enable[0] = true;
            m_presence.cliPreamp1 = true;
        } else if (key == "pre2") {
            m_config.preamp_enable[1] = true;
            m_presence.cliPreamp2 = true;
        }

        // -------- gain --------
        else if (key == "g" || key == "gain") {
            m_config.hi_gain = true;
            m_presence.cliGain = true;
        }

        // -------- polarity --------
        else if (key == "pol1") {
            if (next_value(value)) {
                m_config.polarity[0] = (value == "1");
                m_presence.cliPolarity1 = true;
            }
        } else if (key == "pol2") {
            if (next_value(value)) {
                m_config.polarity[1] = (value == "1");
                m_presence.cliPolarity2 = true;
            }
        }

        // -------- station id --------
        else if (key == "id") {
            if (next_value(value)) {
                m_config.station_ID = value;
                m_presence.cliStationId = true;
            }
        }

        // -------- trigger --------
        else if (key == "t" || key == "trigger") {
            if (next_value(value)) {
                int v = std::stoi(value);
                switch (v) {
                    case 0:
                        m_config.eventTrigger = EVT_XOR;
                        break;
                    case 1:
                        m_config.eventTrigger = EVT_AND;
                        break;
                    case 2:
                        m_config.eventTrigger = TIME_MEAS_OUT;
                        break;
                    case 3:
                        m_config.eventTrigger = EXT_TRIGGER;
                        break;
                    default:
                        break;
                }
                m_presence.cliTrigger = true;
            }
        }

        // -------- pca --------
        else if (key == "pca" || key == "signal") {
            if (next_value(value)) {
                try {
                    m_config.pcaPortMask = static_cast<uint8_t>(std::stoi(value));
                    m_presence.cliTimingInput = true;
                } catch (...) {
                }
            }
        }

        // -------- sds011 --------
        else if (key == "sds011_n_sleep") {
            if (next_value(value)) {
                try {
                    m_config.sds011_n_sleep = static_cast<unsigned>(std::stoi(value));
                    m_presence.cliSds011Sleep = true;
                } catch (...) {
                }
            }
        }

        // -------- help --------
        else if (key == "h" || key == "help") {
            print_help(argv[0]);
            std::exit(0);
        }
    }
}

inline bool readBoolFlexible(const libconfig::Setting& setting) {
    switch (setting.getType()) {
        case libconfig::Setting::TypeBoolean:
            return static_cast<bool>(setting);

        case libconfig::Setting::TypeInt:
            return static_cast<int>(setting) != 0;

        case libconfig::Setting::TypeInt64:
            return static_cast<long long>(setting) != 0;

        default:
            throw libconfig::SettingTypeException(setting);
    }
}

inline long long readIntFlexible(const libconfig::Setting& setting) {
    switch (setting.getType()) {
        case libconfig::Setting::TypeInt:
            return static_cast<long long>(setting);

        case libconfig::Setting::TypeInt64:
            return static_cast<long long>(setting);

        default:
            throw libconfig::SettingTypeException(setting);
    }
}

void ConfigParser::apply_defaults() {

    // Read defaults from SystemConfig

    // Check if data is available
    if (m_config.config_file_data == nullptr || m_config.settings_file_data == nullptr) {
        throw std::runtime_error(
            "CommandlineParser: Could not access config file data or settings data, please load "
            "them to config before initialization.");
    }

    // Load hardware config path
    try {
        m_config.hardwareConfigPath =
            static_cast<std::string>(m_config.config_file_data->lookup("hardware_config"));
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'hardware_config': " + std::string(e.what()));
    }

    // Load sources config path
    try {
        m_config.componentConfigPath =
            static_cast<std::string>(m_config.config_file_data->lookup("component_config"));
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'component_config': " + std::string(e.what()));
    }

    // Load max_geohash_length
    try {
        m_config.maxGeohashLength = m_config.config_file_data->lookup("max_geohash_length");
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'max_geohash_length': " + std::string(e.what()));
    }

    // Load store_local
    try {
        m_config.storeLocal = m_config.config_file_data->lookup("store_local");
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'store_local': " + std::string(e.what()));
    }

    // Load gnss_dynamic_model
    try {
        int model = m_config.config_file_data->lookup("gnss_dynamic_model");
        m_config.gnss_dynamic_model = static_cast<UbxDynamicModel>(model);
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'gnss_dynamic_model': " + std::string(e.what()));
    }

    // Load GPS dev
    try {
        m_config.gpsdevname =
            static_cast<std::string>(m_config.config_file_data->lookup("ublox_device"));
        m_presence.cfgGpsDevice = true;
    } catch (const libconfig::SettingTypeException& e) {
        logWarn("Could not load setting 'ublox_device': " + std::string(e.what()));
    } catch (const libconfig::SettingException& e) {
        // fallback logic
        std::vector<std::string> candidates = {"ttyS0", "ttyAMA0", "serial0"};
        std::vector<std::string> found;

        try {
            for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
                std::string name = entry.path().filename().string();

                if (std::find(candidates.begin(), candidates.end(), name) != candidates.end()) {
                    found.push_back("/dev/" + name);
                }
            }
        } catch (const std::filesystem::filesystem_error& f) {
            logWarn(std::string("Error accessing /dev while probing GNSS devices: ") + f.what());
        }

        if (!found.empty()) {
            m_config.gpsdevname = found.back();
            logInfo("Detected " + m_config.gpsdevname + " as probable GNSS device candidate");
        }
    }

    // Load GPIO dev
    try {
        m_config.gpiodevname =
            static_cast<std::string>(m_config.config_file_data->lookup("gpio_device"));
        m_presence.cfgGpioDevice = true;
    } catch (const libconfig::SettingTypeException& e) {
        logWarn("Could not load setting 'gpio_device': " + std::string(e.what()));
    } catch (const libconfig::SettingException& e) {
        // fallback logic
        std::vector<std::string> candidates = {"gpiochip0"};
        std::vector<std::string> found;

        try {
            for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
                std::string name = entry.path().filename().string();

                if (std::find(candidates.begin(), candidates.end(), name) != candidates.end()) {
                    found.push_back("/dev/" + name);
                }
            }
        } catch (const std::filesystem::filesystem_error& f) {
            logWarn(std::string("Error accessing /dev while probing GPIO devices: ") + f.what());
        }

        if (!found.empty()) {
            m_config.gpiodevname = found.back();
            logInfo("Detected " + m_config.gpiodevname + " as GPIO device candidate");
        }
    }

    // Load ublox_baud
    try {
        m_config.gnss_baudrate = m_config.config_file_data->lookup("ublox_baud");
        m_presence.cfgGnssBaud = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'ublox_baud': " + std::string(e.what()));
    }

    // Load tcp_ip

    try {
        std::string tcpIpCfg = m_config.config_file_data->lookup("tcp_ip");
        m_config.serverAddress = tcpIpCfg;
        m_presence.cfgServerAddress = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'tcp_ip': " + std::string(e.what()));
    }

    // Load tcp_port
    try {
        int port = m_config.config_file_data->lookup("tcp_port");
        m_config.serverPort = static_cast<std::uint16_t>(port);
        m_presence.cfgServerPort = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'tcp_port': " + std::string(e.what()));
    }

    // Load pcaPortmaskCfg
    try {
        auto mask = static_cast<int>(m_config.config_file_data->lookup("timing_input"));
        if (mask < 0 || mask > std::numeric_limits<std::uint8_t>::max()) {
            std::cerr << "Invalid 'timing_input' in SystemConfig file: " << mask << "\n";
        } else {
            m_config.pcaPortMask = mask;
            m_presence.cfgTimingInput = true;
        }
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'timing_input': " + std::string(e.what()));
    }

    // Load biasPowerCfg
    try {
        m_config.bias_ON = readBoolFlexible(m_config.config_file_data->lookup("bias_switch"));
        m_presence.cfgPreamp1 = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'bias_switch': " + std::string(e.what()));
    }

    try {
        m_config.preamp_enable[0] =
            readBoolFlexible(m_config.config_file_data->lookup("preamp1_switch"));
        m_presence.cfgPreamp1 = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'preamp1_switch': " + std::string(e.what()));
    }

    try {
        m_config.preamp_enable[1] =
            readBoolFlexible(m_config.config_file_data->lookup("preamp2_switch"));
        m_presence.cfgPreamp2 = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'preamp2_switch': " + std::string(e.what()));
    }

    try {
        m_config.hi_gain = readBoolFlexible(m_config.config_file_data->lookup("gain_switch"));
        m_presence.cfgGain = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'gain_switch': " + std::string(e.what()));
    }

    int eventTriggerCfg{-1};
    try {
        eventTriggerCfg = m_config.config_file_data->lookup("trigger_input");
        m_presence.cfgTrigger = true;
        switch (eventTriggerCfg) {
            case 0:
                m_config.eventTrigger = EVT_XOR;
                break;
            case 1:
                m_config.eventTrigger = EVT_AND;
                break;
            case 2:
                m_config.eventTrigger = TIME_MEAS_OUT;
                break;
            case 3:
                m_config.eventTrigger = EXT_TRIGGER;
                break;
            default:
                m_config.eventTrigger = EVT_AND;
                break;
        }
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'trigger_input': " + std::string(e.what()));
    }

    // Load pol1Cfg
    try {
        m_config.polarity[0] =
            readBoolFlexible(m_config.config_file_data->lookup("input1_polarity"));
        m_presence.cfgPolarity1 = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'input1_polarity': " + std::string(e.what()));
    }

    // Load pol2Cfg
    try {
        m_config.polarity[1] =
            readBoolFlexible(m_config.config_file_data->lookup("input2_polarity"));
        m_presence.cfgPolarity2 = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'input2_polarity': " + std::string(e.what()));
    }

    // Load mqtt credentials
    try {
        std::string userNameCfg = m_config.config_file_data->lookup("mqtt_user");
        std::string passwordCfg = m_config.config_file_data->lookup("mqtt_password");

        m_config.username = userNameCfg;
        m_config.password = passwordCfg;
    } catch (const libconfig::SettingNotFoundException& nfex) {
        logInfo("No 'mqtt_user' or 'mqtt_password' in config; using previously stored credentials");
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'mqtt_user' or 'mqtt_password': " + std::string(e.what()));
    }

    // Load stationID - Get the station id from config, if it exists
    try {
        std::string stationIdString = m_config.config_file_data->lookup("stationID");
        m_config.station_ID = stationIdString;
        m_presence.cfgStationId = true;
    } catch (const libconfig::SettingNotFoundException&) {
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'mqtt_user' or 'stationID': " + std::string(e.what()));
    }

    // try to read in the stored geo handling fields
    std::string mode_str{};
    try {
        mode_str =
            static_cast<std::string>(m_config.settings_file_data->lookup("geo_handling.mode"));
        logInfo("Position mode from settings: " + mode_str);
    } catch (const libconfig::SettingNotFoundException& e) {
        m_config.position_mode_config.mode = PositionModeConfig::Mode::Auto;
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'mqtt_user' or 'stationID': " + std::string(e.what()));
    }
    if (mode_str ==
        PositionModeConfig::mode_name[static_cast<std::size_t>(PositionModeConfig::Mode::Static)]) {
        m_config.position_mode_config.mode = PositionModeConfig::Mode::Static;
    } else if (mode_str == PositionModeConfig::mode_name[static_cast<std::size_t>(
                               PositionModeConfig::Mode::LockIn)]) {
        m_config.position_mode_config.mode = PositionModeConfig::Mode::LockIn;
    } else {
        m_config.position_mode_config.mode = PositionModeConfig::Mode::Auto;
    }

    try {
        m_config.position_mode_config.static_position.longitude =
            m_config.settings_file_data->lookup("geo_handling.static_coordinates.lon");
        m_config.position_mode_config.static_position.latitude =
            m_config.settings_file_data->lookup("geo_handling.static_coordinates.lat");
        m_config.position_mode_config.static_position.altitude =
            m_config.settings_file_data->lookup("geo_handling.static_coordinates.alt");
        m_config.position_mode_config.static_position.hor_error =
            m_config.settings_file_data->lookup("geo_handling.static_coordinates.hor_error");
        m_config.position_mode_config.static_position.vert_error =
            m_config.settings_file_data->lookup("geo_handling.static_coordinates.vert_error");
        m_presence.cfgPositionMode = true;
    } catch (const libconfig::SettingNotFoundException& e) {
    } catch (const libconfig::SettingException& e) {
        logWarn(std::string(e.what()) + " Could not load setting one of:\n " +
                "'geo_handling.static_coordinates.lon'\n" +
                "'geo_handling.static_coordinates.lat'\n" +
                "'geo_handling.static_coordinates.alt'\n" +
                "'geo_handling.static_coordinates.hor_error'\n" +
                "'geo_handling.static_coordinates.vert_error'");
    }

    // Load sds011_n_sleep - sds011 sleep between readout, 0 for continuous mode
    try {
        int sds_n_sleep = readIntFlexible(m_config.config_file_data->lookup("sds011_n_sleep"));
        if (sds_n_sleep < 0) {
            sds_n_sleep = 1;
        }
        m_config.sds011_n_sleep = static_cast<unsigned>(sds_n_sleep);
    } catch (const libconfig::SettingNotFoundException& e) {
        m_config.sds011_n_sleep = 1;
    } catch (const libconfig::SettingException& e) {
        logWarn("Could not load setting 'sds011_n_sleep': " + std::string(e.what()));
    }
}

bool ConfigParser::is_valid_ipv4(const std::string& ip) {
    sockaddr_in sa{};
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1;
}

void ConfigParser::validate() {
    if (!m_presence.cfgGpsDevice && !m_presence.cliGpsDevice && m_config.gpsdevname.empty()) {
        logWarn(
            "No GNSS device provided by config/settings/CLI; GNSS module will stay disconnected");
    }
    if (!m_presence.cfgGnssBaud && !m_presence.cliGnssBaud) {
        logWarn("No 'ublox_baud' in config and no CLI override; using existing/default baudrate");
    }
    if (!m_presence.cfgServerAddress && !m_presence.cliServerAddress) {
        logWarn("No 'tcp_ip' in config and no CLI override; using existing/default listen address");
    }
    if (!m_presence.cfgServerPort && !m_presence.cliServerPort) {
        logWarn("No 'tcp_port' in config and no CLI override; using existing/default listen port");
    }
    if (!m_presence.cfgTimingInput && !m_presence.cliTimingInput) {
        logWarn(
            "No 'timing_input' in config and no CLI override; using existing/default timing input");
    }
    if (!m_presence.cfgBias && !m_presence.cliBias) {
        logWarn("No 'bias_switch' in config and no CLI override; using existing/default bias "
                "switch state");
    }
    if (!m_presence.cfgPreamp1 && !m_presence.cliPreamp1) {
        logWarn("No 'preamp1_switch' in config and no CLI override; using existing/default preamp1 "
                "state");
    }
    if (!m_presence.cfgPreamp2 && !m_presence.cliPreamp2) {
        logWarn("No 'preamp2_switch' in config and no CLI override; using existing/default preamp2 "
                "state");
    }
    if (!m_presence.cfgGain && !m_presence.cliGain) {
        logWarn(
            "No 'gain_switch' in config and no CLI override; using existing/default gain state");
    }
    if (!m_presence.cfgTrigger && !m_presence.cliTrigger) {
        logWarn("No 'trigger_input' in config and no CLI override; using existing/default trigger "
                "mode");
    }
    if (!m_presence.cfgPolarity1 && !m_presence.cliPolarity1) {
        logWarn("No 'input1_polarity' in config and no CLI override; using existing/default "
                "polarity for channel 1");
    }
    if (!m_presence.cfgPolarity2 && !m_presence.cliPolarity2) {
        logWarn("No 'input2_polarity' in config and no CLI override; using existing/default "
                "polarity for channel 2");
    }
    if (!m_presence.cfgStationId && !m_presence.cliStationId) {
        logWarn("No 'stationID' in config and no CLI override; using existing/default station ID");
    }
    if (!m_presence.cfgSds011Devname && !m_presence.cliSds011Devname) {
        logWarn("No 'sds011_device' in config and no CLI override; using existing/default: "
                "'/dev/ttyUSB0'");
    }
    if (!m_presence.cfgSds011Baudrate && !m_presence.cliSds011Baudrate) {
        logWarn("No 'sds011_baudrate' in config and no CLI override; using existing/default: 9600");
    }
    if (!m_presence.cfgSds011Sleep && !m_presence.cliSds011Sleep) {
        logWarn("No 'sds011_sleep' in config and no CLI override; using existing/default: "
                "continuous every 1s readout");
    }

    // Validate IP
    if (!is_valid_ipv4(m_config.serverAddress)) {
        if (m_config.serverAddress != "localhost" && m_config.serverAddress != "local") {
            m_config.serverAddress.clear();
            logError("Invalid daemon IP address; not a valid IPv4 address");
        }
    }
}

void ConfigParser::report() {
    logInfo("ublox baudrate: " + std::to_string(m_config.gnss_baudrate));
    logInfo("ublox device: " + m_config.gpsdevname);
    logInfo("tcp_ip (listen ip): " + m_config.serverAddress);
    logInfo("tcp_port (listen port): " + std::to_string(m_config.serverPort));
    logInfo("timing input: " + std::to_string(m_config.pcaPortMask));
    logInfo("bias switch: " + std::to_string(m_config.bias_ON));
    logInfo("preamp1 switch: " + std::to_string(m_config.preamp_enable[0]));
    logInfo("preamp2 switch: " + std::to_string(m_config.preamp_enable[1]));
    logInfo("gain switch: " + std::to_string(m_config.hi_gain));
    logInfo("input polarity ch1: " + std::to_string(m_config.polarity[0]));
    logInfo("input polarity ch2: " + std::to_string(m_config.polarity[1]));
    logInfo("mqtt user: " + m_config.username + " passw: " + m_config.password);
    logInfo("station id: " + m_config.station_ID);
    logInfo("sds011_baudrate: " + std::to_string(m_config.sds011_baudrate));
    logInfo("sds011_device: " + m_config.sds011_devname);
    logInfo("sds011_sleep: " + std::to_string(m_config.sds011_n_sleep * 60 - 30) + "s");
}
