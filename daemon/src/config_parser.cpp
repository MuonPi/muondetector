#include "config_parser.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <vector>
#include <arpa/inet.h>
#include <libconfig.h++>


ConfigParser::ConfigParser(int argc, char *argv[], std::shared_ptr<configuration> f_config) : m_config{f_config}
{
    apply_defaults();
    parse(argc, argv);
    validate();
    report();
}

ConfigParser::~ConfigParser()
{
}

auto ConfigParser::get() const -> const std::shared_ptr<configuration>
{
    return m_config;
}

bool ConfigParser::is_flag(const std::string &arg)
{
    return arg.rfind("-", 0) == 0;
}

std::string ConfigParser::strip_prefix(const std::string &arg)
{
    if (arg.rfind("--", 0) == 0)
        return arg.substr(2);
    if (arg.rfind("-", 0) == 0)
        return arg.substr(1);
    return arg;
}

void ConfigParser::parse(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        // -------- positional (device) --------
        if (!is_flag(arg))
        {
            if (m_config->gpsdevname.empty())
                m_config->gpsdevname = arg;
            continue;
        }

        std::string key = strip_prefix(arg);

        auto next_value = [&](std::string &out) -> bool {
            if (i + 1 < argc && !is_flag(argv[i + 1]))
            {
                out = argv[++i];
                return true;
            }
            return false;
        };

        std::string value;

        // -------- verbosity --------
        if (key == "e" || key == "verbose")
        {
            if (next_value(value))
            {
                try
                {
                    m_config->verbose = std::stoi(value);
                }
                catch (...)
                {
                    m_config->verbose = 0;
                }
            }
        }

        // -------- dump raw --------
        else if (key == "d")
        {
            m_config->gnss_dump_raw = true;
        }

        // -------- show in/out --------
        else if (key == "showin" || key == "showincoming")
        {
            m_config->showin = true;
        }
        else if (key == "showout" || key == "showoutgoing")
        {
            m_config->showout = true;
        }

        // -------- baudrate --------
        else if (key == "b")
        {
            if (next_value(value))
            {
                try
                {
                    m_config->gnss_baudrate = std::stoi(value);
                }
                catch (...)
                {
                }
            }
        }

        // -------- server --------
        else if (key == "server" || key == "daemonAddress")
        {
            if (next_value(value))
            {
                m_config->serverAddress = value;
            }
        }

        else if (key == "dp" || key == "daemonPort")
        {
            if (next_value(value))
            {
                try
                {
                    m_config->serverPort = static_cast<uint16_t>(std::stoi(value));
                }
                catch (...)
                {
                }
            }
        }

        // -------- thresholds --------
        else if (key == "th1" || key == "discr1")
        {
            if (next_value(value))
            {
                try
                {
                    m_config->thresholdVoltage[0] = std::stof(value);
                }
                catch (...)
                {
                }
            }
        }

        else if (key == "th2" || key == "discr2")
        {
            if (next_value(value))
            {
                try
                {
                    m_config->thresholdVoltage[1] = std::stof(value);
                }
                catch (...)
                {
                }
            }
        }

        // -------- bias --------
        else if (key == "bias")
        {
            if (next_value(value))
            {
                try
                {
                    m_config->biasVoltage = std::stof(value);
                }
                catch (...)
                {
                }
            }
        }

        else if (key == "p")
        {
            m_config->bias_ON = true;
        }

        // -------- preamps --------
        else if (key == "pre1")
        {
            m_config->preamp_enable[0] = true;
        }
        else if (key == "pre2")
        {
            m_config->preamp_enable[1] = true;
        }

        // -------- gain --------
        else if (key == "g" || key == "gain")
        {
            m_config->hi_gain = true;
        }

        // -------- polarity --------
        else if (key == "pol1")
        {
            if (next_value(value))
            {
                m_config->polarity[0] = (value == "1");
            }
        }
        else if (key == "pol2")
        {
            if (next_value(value))
            {
                m_config->polarity[1] = (value == "1");
            }
        }

        // -------- station id --------
        else if (key == "id")
        {
            if (next_value(value))
            {
                m_config->station_ID = value;
            }
        }

        // -------- trigger --------
        else if (key == "t" || key == "trigger")
        {
            if (next_value(value))
            {
                int v = std::stoi(value);
                switch (v)
                {
                case 0:
                    m_config->eventTrigger = EVT_XOR;
                    break;
                case 1:
                    m_config->eventTrigger = EVT_AND;
                    break;
                case 2:
                    m_config->eventTrigger = TIME_MEAS_OUT;
                    break;
                case 3:
                    m_config->eventTrigger = EXT_TRIGGER;
                    break;
                default:
                    break;
                }
            }
        }

        // -------- pca --------
        else if (key == "pca" || key == "signal")
        {
            if (next_value(value))
            {
                try
                {
                    m_config->pcaPortMask = static_cast<uint8_t>(std::stoi(value));
                }
                catch (...)
                {
                }
            }
        }
    }
}

void ConfigParser::apply_defaults()
{

    // Read defaults from configuration

    // Check if data is available
    if (m_config->config_file_data == nullptr || m_config->settings_file_data == nullptr)
    {
        throw std::runtime_error("CommandlineParser: Could not access config file data or settings data, please load "
                                 "them to config before initialization.");
    }

    // Load max_geohash_length
    try
    {
        m_config->maxGeohashLength = m_config->config_file_data->lookup("max_geohash_length");
    }
    catch (const libconfig::SettingNotFoundException &)
    {
    }

    // Load store_local
    try
    {
        m_config->storeLocal = m_config->config_file_data->lookup("store_local");
    }
    catch (const libconfig::SettingNotFoundException &)
    {
    }

    // Load gnss_dynamic_model
    try
    {
        int model = m_config->config_file_data->lookup("gnss_dynamic_model");
        m_config->gnss_dynamic_model = static_cast<UbxDynamicModel>(model);
    }
    catch (const libconfig::SettingNotFoundException &)
    {
    }

    // Load GPS dev
    try
    {
        m_config->gpsdevname = static_cast<std::string>(m_config->config_file_data->lookup("ublox_device"));
    }
    catch (const libconfig::SettingNotFoundException &)
    {
        std::cerr << "No 'ublox_device' setting in configuration file. Will guess...\n";

        std::vector<std::string> candidates = {"ttyS0", "ttyAMA0", "serial0"};

        std::vector<std::string> found;

        try
        {
            for (const auto &entry : std::filesystem::directory_iterator("/dev"))
            {
                std::string name = entry.path().filename().string();

                if (std::find(candidates.begin(), candidates.end(), name) != candidates.end())
                {
                    found.push_back("/dev/" + name);
                }
            }
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Error accessing /dev: " << e.what() << '\n';
        }

        if (!found.empty())
        {
            m_config->gpsdevname = found.back(); // mimic Qt "last()"
            std::cout << "detected " << m_config->gpsdevname << " as most probable candidate\n";
        }
        else
        {
            std::cerr << "no device selected, will not connect to GNSS module\n";
        }
    }

    // Load ublox_baud
    try
    {
        m_config->gnss_baudrate = m_config->config_file_data->lookup("ublox_baud");
    }
    catch (const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No 'ublox_baud' setting in configuration file. Assuming" << m_config->gnss_baudrate;
    }

    // Load tcp_ip

    try
    {
        std::string tcpIpCfg = m_config->config_file_data->lookup("tcp_ip");
        m_config->serverAddress = tcpIpCfg;
    }
    catch (const libconfig::SettingNotFoundException &)
    {
    }

    // Load tcp_port
    try
    {
        int port = m_config->config_file_data->lookup("tcp_port");
        m_config->serverPort = static_cast<std::uint16_t>(port);
    }
    catch (const libconfig::SettingNotFoundException &)
    {
    }

    // Load pcaPortmaskCfg
    try
    {
        auto mask = static_cast<int>(m_config->config_file_data->lookup("timing_input"));
        if (mask < 0 || mask > std::numeric_limits<std::uint8_t>::max())
        {
            std::cerr << "Invalid 'timing_input' in configuration file: " << mask;
        }
        else
        {
            m_config->pcaPortMask = mask;
        }
    }
    catch (const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No 'timing_input' setting in configuration file. Assuming" << static_cast<unsigned>(m_config->pcaPortMask);
    }

    // Load biasPowerCfg
    try
    {
        m_config->bias_ON = m_config->config_file_data->lookup("bias_switch");
    }
    catch (const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No 'bias_switch' setting in configuration file. Assuming" << (int)m_config->bias_ON;
    }

    try
    {
        m_config->preamp_enable[0] = m_config->config_file_data->lookup("preamp1_switch");
    }
    catch (const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No 'preamp1_switch' setting in configuration file. Assuming" << (int)m_config->preamp_enable[0];
    }

    try
    {
        m_config->preamp_enable[1] = m_config->config_file_data->lookup("preamp2_switch");
    }
    catch (const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No 'preamp2_switch' setting in configuration file. Assuming " << (int)m_config->preamp_enable[1];
    }

    try
    {
        m_config->hi_gain = m_config->config_file_data->lookup("gain_switch");
    }
    catch (const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No 'gain_switch' setting in configuration file. Assuming" << (int)m_config->hi_gain;
    }

    int eventTriggerCfg{-1};
    try
    {
        eventTriggerCfg = m_config->config_file_data->lookup("trigger_input");
        std::cout << "event trigger : " << eventTriggerCfg;
    }
    catch (const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No 'trigger_input' setting in configuration file. Assuming signal"
                  << GPIO_SIGNAL_MAP.at(m_config->eventTrigger).name;
    }

    switch (eventTriggerCfg)
    {
    case 0:
        m_config->eventTrigger = EVT_XOR;
        break;
    case 1:
        m_config->eventTrigger = EVT_AND;
        break;
    case 2:
        m_config->eventTrigger = TIME_MEAS_OUT;
        break;
    case 3:
        m_config->eventTrigger = EXT_TRIGGER;
        break;
    default:
        m_config->eventTrigger = EVT_AND;
        break;
    }

    // Load pol1Cfg
    try
    {
        m_config->polarity[0] = m_config->config_file_data->lookup("input1_polarity");
    }
    catch (const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No 'input1_polarity' setting in configuration file. Assuming" << (int)m_config->polarity[0];
    }

    // Load pol2Cfg
    try
    {
        m_config->polarity[1] = m_config->config_file_data->lookup("input2_polarity");
    }
    catch (const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No 'input2_polarity' setting in configuration file. Assuming" << (int)m_config->polarity[1];
    }

    // Load mqtt credentials
    try {
        std::string userNameCfg = m_config->config_file_data->lookup("mqtt_user");
        std::string passwordCfg = m_config->config_file_data->lookup("mqtt_password");

        m_config->username = userNameCfg;
        m_config->password = passwordCfg;
    } catch (const libconfig::SettingNotFoundException& nfex) {
        std::cout << "No 'mqtt_user' or 'mqtt_password' setting in configuration file. Will continue with previously stored credentials";
    }


    // Load stationID - Get the station id from config, if it exists
    try {
        std::string stationIdString = m_config->config_file_data->lookup("stationID");
        m_config->station_ID = stationIdString;
    } catch (const libconfig::SettingNotFoundException& nfex) {
        std::cerr << "No 'stationID' setting in configuration file. Assuming stationID='0'";
    }

    // try to read in the stored geo handling fields
    std::string mode_str = m_config->settings_file_data->lookup("geo_handling.mode");
    std::cout << "mode = " << mode_str;
    if (mode_str == PositionModeConfig::mode_name[static_cast<std::size_t>(PositionModeConfig::Mode::Static)]) {
        m_config->position_mode_config.mode = PositionModeConfig::Mode::Static;
    } else if (mode_str == PositionModeConfig::mode_name[static_cast<std::size_t>(PositionModeConfig::Mode::LockIn)]) {
        m_config->position_mode_config.mode = PositionModeConfig::Mode::LockIn;
    } else {
        m_config->position_mode_config.mode = PositionModeConfig::Mode::Auto;
    }
    m_config->position_mode_config.static_position.longitude = m_config->settings_file_data->lookup("geo_handling.static_coordinates.lon");
    m_config->position_mode_config.static_position.latitude = m_config->settings_file_data->lookup("geo_handling.static_coordinates.lat");
    m_config->position_mode_config.static_position.altitude = m_config->settings_file_data->lookup("geo_handling.static_coordinates.alt");
    m_config->position_mode_config.static_position.hor_error = m_config->settings_file_data->lookup("geo_handling.static_coordinates.hor_error");
    m_config->position_mode_config.static_position.vert_error = m_config->settings_file_data->lookup("geo_handling.static_coordinates.vert_error");
}

bool ConfigParser::is_valid_ipv4(const std::string& ip)
{
    sockaddr_in sa{};
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1;
}

void ConfigParser::validate()
{
    // Validate IP
    if (!is_valid_ipv4(m_config->serverAddress))
    {
        if (m_config->serverAddress != "localhost" &&
            m_config->serverAddress != "local")
        {
            m_config->serverAddress.clear();
            std::cerr << "wrong daemon ipAddress, not an ipv4address\n";
        }
    }
}

void ConfigParser::report()
{

    if (m_config->verbose > 2)
    {
        std::cout << "ublox baudrate:" << m_config->gnss_baudrate << '\n';
        std::cout << "ublox device: " << m_config->gpsdevname << '\n';
        std::cout << "tcp_ip (listen ip): " << m_config->serverAddress << '\n';
        std::cout << "tcp_port (listen port): " << m_config->serverPort << '\n';
        std::cout << "timing input: " << m_config->pcaPortMask << '\n';
        std::cout << "bias switch:" << m_config->bias_ON << '\n';
        std::cout << "preamp1 switch:" << m_config->preamp_enable[0] << '\n';
        std::cout << "preamp2 switch:" << m_config->preamp_enable[1] << '\n';
        std::cout << "gain switch:" << m_config->hi_gain << '\n';
        std::cout << "input polarity ch1:" << m_config->polarity[0] << '\n';
        std::cout << "input polarity ch2:" << m_config->polarity[1] << '\n';
        std::cout << "mqtt user: " << m_config->username << " passw: " << m_config->password << '\n';
    }
    if (m_config->verbose) {
        std::cout << "station id: " << m_config->station_ID << '\n';
    }
    std::cout << std::flush;
}