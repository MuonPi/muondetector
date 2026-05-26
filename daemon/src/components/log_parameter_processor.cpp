#include "components/log_parameter_processor.h"

// Core components
#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/component_manager.h"
#include "core/registries/data_store.h"
#include "drivers/ads1115_driver.h"

// UBLOX
#include "data/events/ubx_event.h"
#include "data/ublox/ublox_messages.h"
#include "data/ublox/ublox_structs.h"

// I2C events
#include "data/events/ads1115_event.h"
#include "data/events/bias_current_event.h"
#include "data/events/bias_switch_event.h"
#include "data/events/bias_voltage_event.h"
#include "data/events/event_trigger_event.h"
#include "data/events/gain_switch_event.h"
#include "data/events/mcp4728_event.h"
#include "data/events/pca_switch_event.h"
#include "data/events/polarity_switch_event.h"
#include "data/events/preamp_switch_event.h"
#include "data/events/temperature_event.h"
#include "data/events/threshold_setting_event.h"

// GPIO events
#include "data/events/gpio_event.h"
#include "data/events/gpio_rate_event.h"

// Utility
#include "data/custom_io_operators.h"
#include "sys/sysinfo.h"
#include "utility/calibration.h"
#include "utility/conversion.h"
#include "utility/geohash.h"
#include "utility/logparameter.h"

#include <format>
#include <string>

void LogParameterProcessor::setup(EventBus& bus, DataStore& datastore) {
    bus.subscribe<UbxTimeMarkStruct>([&bus](const UbxTimeMarkStruct& tm) {
        if (!tm.risingValid && !tm.fallingValid) {
            logDebug("Daemon::onUBXReceivedTimeTM2(const UbxTimeMarkStruct&): detected invalid "
                     "time mark message; no rising or falling edge data");
            return;
        }
        // static UbxTimeMarkStruct lastTimeMark {};
        using namespace std::chrono;
        auto systime{system_clock::now().time_since_epoch()};
        auto gpstime{
            duration_cast<nanoseconds>(nanoseconds(tm.rising.tv_nsec) + seconds(tm.rising.tv_sec))};
        auto difftime{systime - gpstime};
        // std::cout<<std::dec<<"Tdiff: "<<difftime.count()*1e-9<<" s\n";
        bus.publish(LogParameter("gnssTimeOffset",
                                 std::format("{:.3f}", difftime.count() * 1e-9) + " s",
                                 LogParameter::LOG_LATEST));

        // long double dts = (tm.falling.tv_sec - tm.rising.tv_sec) * 1.0e9L;
        // dts += (tm.falling.tv_nsec - tm.rising.tv_nsec);
        // if ((dts > 0.0L) && tm.fallingValid) {
        //     m_histo_map["UbxEventLength"]->fill(static_cast<double>(dts));
        // }
        // long double interval = (tm.rising.tv_sec - lastTimeMark.rising.tv_sec) * 1.0e9L;
        // interval += (tm.rising.tv_nsec - lastTimeMark.rising.tv_nsec);
        // if (interval < 1e12)
        //     m_histo_map["UbxEventInterval"]->fill(static_cast<double>(1.0e-6L * interval));
        // uint16_t diffCount = tm.evtCounter - lastTimeMark.evtCounter;
        // emit timeMarkIntervalCountUpdate(diffCount, static_cast<double>(interval * 1.0e-9L));
        // lastTimeMark = tm;

        // output is: rising falling timeAcc valid timeBase utcAvailable
        std::stringstream tempStream;
        tempStream << tm.rising << tm.falling << tm.accuracy_ns << " " << tm.evtCounter << " "
                   << static_cast<short>(tm.valid) << " " << static_cast<short>(tm.timeBase) << " "
                   << static_cast<short>(tm.utcAvailable);
        bus.publish(LogParameter("ubloxCounter", std::to_string(tm.evtCounter) + " ",
                                 LogParameter::LOG_LATEST));

        if (!tm.risingValid || !tm.fallingValid) {
            logDebug("detected timemark message with reconstructed edge time (" +
                     std::string((tm.risingValid) ? "falling" : "rising") +
                     ")\nmsg: " + tempStream.str());
        }
    });

    bus.subscribe<UbxDopStruct>([&bus](const UbxDopStruct& dops) {
        bus.publish(LogParameter{"positionDOP", std::to_string(dops.pDOP / 100.),
                                 LogParameter::LOG_AVERAGE});
        bus.publish(
            LogParameter{"timeDOP", std::to_string(dops.tDOP / 100.), LogParameter::LOG_AVERAGE});
    });
    bus.subscribe<GpioRateEvent>([&bus](const GpioRateEvent& event) {
        if (event.rate.empty()) {
            return;
        }
        switch (event.whichRate) {
            case 0:
                bus.publish(LogParameter{"rateXOR", std::to_string(event.rate.at(0).second) + " Hz",
                                         LogParameter::LOG_AVERAGE});
                break;
            case 1:
                bus.publish(LogParameter{"rateAND", std::to_string(event.rate.at(0).second) + " Hz",
                                         LogParameter::LOG_AVERAGE});
                break;
        }
    });
    bus.subscribe<ADS1115Event>([&bus](const ADS1115Event& event) {
        bus.publish(LogParameter{"adcSamplingTime", std::to_string(event.convTime) + " ms",
                                 LogParameter::LOG_AVERAGE});
    });
    bus.subscribe<GeoPosition>([&bus](const GeoPosition& pos) {
        std::string geohash = GeoHash::hashFromCoordinates(pos.longitude, pos.latitude, 10);
        bus.publish(LogParameter{"geoLongitude", std::to_string(pos.longitude) + " deg",
                                 LogParameter::LOG_AVERAGE});

        bus.publish(LogParameter{"geoLongitude", std::to_string(pos.longitude) + " deg",
                                 LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"geoLatitude", std::to_string(pos.latitude) + " deg",
                                 LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"geoHash", geohash + " ", LogParameter::LOG_LATEST});
        bus.publish(LogParameter{"geoHeightMSL", std::to_string(pos.altitude) + " m",
                                 LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"geoHorAccuracy", std::to_string(pos.hor_error) + " m",
                                 LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"geoVertAccuracy", std::to_string(pos.vert_error) + " m",
                                 LogParameter::LOG_AVERAGE});
    });
    bus.subscribe<GpioEvent>([&bus](const GpioEvent& event) {
        switch (event.gpio_signal) {
            case GPIO_SIGNAL::PREAMP_1:
                bus.publish(LogParameter{"preampSwitch1",
                                         std::to_string(static_cast<unsigned>(event.edge)),
                                         LogParameter::LOG_EVERY});
                break;
            case GPIO_SIGNAL::PREAMP_2:
                bus.publish(LogParameter{"preampSwitch2",
                                         std::to_string(static_cast<unsigned>(event.edge)),
                                         LogParameter::LOG_EVERY});
                break;
            case GPIO_SIGNAL::IN_POL1:
                bus.publish(LogParameter{"polaritySwitch1",
                                         std::to_string(static_cast<unsigned>(event.edge)),
                                         LogParameter::LOG_EVERY});
                break;
            case GPIO_SIGNAL::IN_POL2:
                bus.publish(LogParameter{"polaritySwitch2",
                                         std::to_string(static_cast<unsigned>(event.edge)),
                                         LogParameter::LOG_EVERY});
                break;
            case GPIO_SIGNAL::GAIN_HL:
                bus.publish(LogParameter{"gainSwitch",
                                         std::to_string(static_cast<unsigned>(event.edge)),
                                         LogParameter::LOG_EVERY});
                break;
            case GPIO_SIGNAL::STATUS1:
                bus.publish(LogParameter{"status1",
                                         std::to_string(static_cast<unsigned>(event.edge)),
                                         LogParameter::LOG_EVERY});
                break;
            case GPIO_SIGNAL::STATUS2:
                bus.publish(LogParameter{"status2",
                                         std::to_string(static_cast<unsigned>(event.edge)),
                                         LogParameter::LOG_EVERY});
                break;
            case GPIO_SIGNAL::STATUS3:
                bus.publish(LogParameter{"status3",
                                         std::to_string(static_cast<unsigned>(event.edge)),
                                         LogParameter::LOG_EVERY});
                break;
            default:
                break;
        }
    });
    bus.subscribe<BiasVoltageEvent>([&bus](const BiasVoltageEvent& event) {
        bus.publish(
            LogParameter{"biasDAC", std::to_string(event.voltage) + " V", LogParameter::LOG_EVERY});
    });
    bus.subscribe<BiasSwitchEvent>([&bus](const BiasSwitchEvent& event) {
        bus.publish(
            LogParameter{"biasSwitch", std::to_string(event.biasOn), LogParameter::LOG_EVERY});
    });
    bus.subscribe<ThresholdSettingEvent>([&bus](const ThresholdSettingEvent& event) {
        bus.publish(LogParameter{"thresh" + std::to_string(event.channel + 1),
                                 std::to_string(event.voltage) + " V", LogParameter::LOG_EVERY});
    });
    bus.subscribe<GnssMonHwStruct>([&bus](const GnssMonHwStruct& hw) {
        bus.publish(LogParameter{"preampNoise", std::to_string(-hw.noisePerMS) + " dBHz",
                                 LogParameter::LOG_AVERAGE});
        bus.publish(
            LogParameter{"preampAGC", std::to_string(hw.agcCnt), LogParameter::LOG_AVERAGE});
        bus.publish(
            LogParameter{"antennaStatus", std::to_string(hw.antStatus), LogParameter::LOG_LATEST});
        bus.publish(
            LogParameter{"antennaPower", std::to_string(hw.antPower), LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"jammingLevel", std::to_string(hw.jamInd / 2.55) + " %",
                                 LogParameter::LOG_AVERAGE});
    });
    bus.subscribe<NavSat>([&bus](const NavSat& event) {
        std::vector<GnssSatellite> visibleSats = event.satellites;
        std::sort(visibleSats.begin(), visibleSats.end(), GnssSatellite::sortByCnr);
        while (visibleSats.size() > 0 && visibleSats.back().Cnr == 0) {
            visibleSats.pop_back();
        }
        std::size_t usedSats = 0, maxCnr = 0;
        if (visibleSats.size()) {
            for (auto sat : visibleSats) {
                if (sat.Used)
                    usedSats++;
                if (sat.Cnr > maxCnr)
                    maxCnr = sat.Cnr;
            }
        }
        bus.publish(
            LogParameter{"sats", std::to_string(event.goodSats), LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"usedSats", std::to_string(usedSats), LogParameter::LOG_AVERAGE});
        bus.publish(
            LogParameter{"maxCNR", std::to_string(maxCnr) + " dB", LogParameter::LOG_AVERAGE});
    });
    bus.subscribe<MonTx>([&bus](const MonTx& event) {
        bus.publish(LogParameter("TXBufUsage", std::to_string(event.tUsage) + " %",
                                 LogParameter::LOG_AVERAGE));
        bus.publish(LogParameter("maxTXBufUsage", std::to_string(event.tPeakUsage) + " %",
                                 LogParameter::LOG_LATEST));
    });
    bus.subscribe<MonRx>([&bus](const MonRx& event) {
        bus.publish(LogParameter("RXBufUsage", std::to_string(event.tUsage) + " %",
                                 LogParameter::LOG_AVERAGE));
        bus.publish(LogParameter("maxRXBufUsage", std::to_string(event.tPeakUsage) + " %",
                                 LogParameter::LOG_LATEST));
    });
    bus.subscribe<NavClock>([&bus](const NavClock& event) {
        bus.publish(LogParameter{"timeAccuracy", std::to_string(event.tAcc) + " ns",
                                 LogParameter::LOG_LATEST});
        bus.publish(LogParameter{"freqAccuracy", std::to_string(event.fAcc) + " ps/s",
                                 LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"clockDrift", std::to_string(event.clkD) + " ns/s",
                                 LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"clockBias", std::to_string(event.clkB) + " ns",
                                 LogParameter::LOG_AVERAGE});
    });
    bus.subscribe<NavStatus>([&bus](const NavStatus& event) {
        bus.publish(
            LogParameter{"fixStatus", std::to_string(event.gpsFix), LogParameter::LOG_LATEST});
        bus.publish(LogParameter{"fixStatusString", (Gnss::FixType::name[event.gpsFix]),
                                 LogParameter::LOG_LATEST});
        bus.publish(LogParameter{"ubloxUptime", std::to_string(event.msss / 1000) + " s",
                                 LogParameter::LOG_LATEST});
    });
    bus.subscribe<GpsVersion>([&bus](const GpsVersion& event) {
        bus.publish(
            LogParameter("UBX_SW_Version", "\"" + event.swString + "\"", LogParameter::LOG_ONCE));
        bus.publish(
            LogParameter("UBX_HW_Version", "\"" + event.hwString + "\"", LogParameter::LOG_ONCE));
    });
    bus.subscribe<UbxProtVersion>([&bus](const UbxProtVersion& event) {
        bus.publish(LogParameter("UBX_Prot_Version",
                                 std::to_string(event.major) + "." + std::to_string(event.minor),
                                 LogParameter::LOG_ONCE));
    });
    bus.subscribe<TemperatureEvent>([&bus](const TemperatureEvent& event) {
        bus.publish(LogParameter{"temperature_" + event.source,
                                 std::to_string(event.temperature) + " degC",
                                 LogParameter::LOG_AVERAGE});
    });
    bus.subscribe<PcaSwitchEvent>([&bus](const PcaSwitchEvent& event) {
        bus.publish(LogParameter{"ubxInputSwitch", "0x" + to_hex(event.pcaPortMask),
                                 LogParameter::LOG_EVERY});
    });

    // retrieve ShowerDetectorCalib from datastore and log eeprom ID
    auto* ptr = datastore.get<std::shared_ptr<ShowerDetectorCalib>>();
    if (ptr == nullptr) {
        return;
    }
    auto weak_ptr = std::weak_ptr<ShowerDetectorCalib>(*ptr);
    if (auto calib = weak_ptr.lock()) {
        bus.publish(LogParameter{"uniqueId", to_hex(calib->getSerialID()), LogParameter::LOG_ONCE});
    }
}

// On each polling, accumulate all logging data
void LogParameterProcessor::poll(EventBus& bus, const DataStore& datastore,
                                 ComponentManager& components) {
    struct sysinfo info;
    memset(&info, 0, sizeof info);
    int err = sysinfo(&info);
    if (!err) {
        double f_load = 1.0 / (1 << SI_LOAD_SHIFT);
        std::stringstream sstr;
        sstr << "*** Sysinfo Stats ***";
        sstr << "nr of cpus      :" << get_nprocs();
        sstr << "uptime (h)      :" << info.uptime / 3600.;
        sstr << "load avg (1min) :" << info.loads[0] * f_load;
        sstr << "free RAM        :" << (1.0e-6 * info.freeram / info.mem_unit) << "Mb";
        sstr << "free swap       :" << (1.0e-6 * info.freeswap / info.mem_unit) << "Mb";
        logDebug(sstr.str());
        bus.publish(LogParameter{"systemNrCPUs", std::to_string(get_nprocs()) + " ",
                                 LogParameter::LOG_ONCE});
        bus.publish(LogParameter{"systemUptime", std::to_string(info.uptime / 3600.) + " h",
                                 LogParameter::LOG_LATEST});
        bus.publish(LogParameter{"systemFreeMem",
                                 std::to_string(1e-6 * info.freeram / info.mem_unit) + " Mb",
                                 LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"systemFreeSwap",
                                 std::to_string(1e-6 * info.freeswap / info.mem_unit) + " Mb",
                                 LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"systemLoadAvg", std::to_string(info.loads[0] * f_load) + " ",
                                 LogParameter::LOG_AVERAGE});
    }

    // retrieve voltages from adc
    auto adc = components.get<ADS1115Driver>(Device::ADS1115_0);
    if (adc == nullptr) {
        return;
    }
    bool ok;
    double v1{adc->getVoltage(MuonPi::Config::Hardware::ADC::Channel::bias1, ok)};
    if (!ok) {
        return;
    }
    double v2{adc->getVoltage(MuonPi::Config::Hardware::ADC::Channel::bias2, ok)};
    if (!ok) {
        return;
    }
    // retrieve calibration from datastore and apply to voltages
    auto* raw = datastore.get<std::shared_ptr<ShowerDetectorCalib>>();
    if (raw == nullptr) {
        return;
    }
    auto calib_weak = std::weak_ptr<ShowerDetectorCalib>(*raw);
    if (auto calib = calib_weak.lock()) {
        if (calib->getCalibItem("VDIV").name != "VDIV") {
            return;
        }
        CalibStruct vdivItem = calib->getCalibItem("VDIV");
        std::istringstream istr(vdivItem.value);
        double vdiv;
        istr >> vdiv;
        vdiv /= 100.;
        bus.publish(LogParameter{"calib_vdiv", std::to_string(vdiv), LogParameter::LOG_ONCE});
        istr.clear();
        istr.str(calib->getCalibItem("RSENSE").value);
        double rsense;
        istr >> rsense;
        logDebug("rsense:" + calib->getCalibItem("RSENSE").value + " (" + std::to_string(rsense) +
                 ")");
        rsense /= 10. * 1000.; // yields Rsense in MOhm
        bus.publish(LogParameter{"calib_rsense", std::to_string(rsense * 1000.) + " kOhm",
                                 LogParameter::LOG_ONCE});
        double ubias = v2 * vdiv;
        bus.publish(LogParameter{"vbias", std::to_string(ubias) + " V", LogParameter::LOG_AVERAGE});
        double usense = (v1 - v2) * vdiv;
        bus.publish(
            LogParameter{"vsense", std::to_string(usense) + " V", LogParameter::LOG_AVERAGE});

        CalibStruct flagItem = calib->getCalibItem("CALIB_FLAGS");
        int calFlags = 0;

        istr.clear();
        istr.str(flagItem.value);
        istr >> calFlags;
        double icorr = 0.;
        if (calFlags & CalibStruct::CALIBFLAGS_CURRENT_COEFFS) {
            double islope, ioffs;
            istr.clear();
            istr.str(calib->getCalibItem("COEFF2").value);
            istr >> ioffs;
            bus.publish(
                LogParameter{"calib_coeff2", std::to_string(ioffs), LogParameter::LOG_ONCE});
            istr.clear();
            istr.str(calib->getCalibItem("COEFF3").value);
            istr >> islope;
            bus.publish(
                LogParameter{"calib_coeff3", std::to_string(islope), LogParameter::LOG_ONCE});
            icorr = ubias * islope + ioffs;
        }
        double ibias = usense / rsense - icorr;
        bus.publish(BiasCurrentEvent{ibias});
        bus.publish(
            LogParameter{"ibias", std::to_string(ibias) + " uA", LogParameter::LOG_AVERAGE});

    } else {
        bus.publish(LogParameter{"vadc3", std::to_string(v1) + " V", LogParameter::LOG_AVERAGE});
        bus.publish(LogParameter{"vadc4", std::to_string(v2) + " V", LogParameter::LOG_AVERAGE});
    }
}