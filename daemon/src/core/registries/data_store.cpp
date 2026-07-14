#include "core/registries/data_store.h"

#include "config.h"
#include "core/logging/logger.h"
#include "data/events/ads1115_event.h"
#include "data/events/bias_current_event.h"
#include "data/events/event_trigger_event.h"
#include "data/events/gpio_event.h"
#include "data/events/interval_event.h"
#include "data/events/ubx_event.h"
#include "data/ublox/ublox_messages.h"
#include "utility/calibration.h"
#include "utility/geoposmanager.h"
#include "utility/ublox_ratebuffer.h"
// #include "data/"

#include <cmath>

import muondetector.histogram;

DataStore::DataStore()
    : m_geopos_manager{std::make_unique<GeoPosManager>()}
    , m_ublox_ratebuffer{std::make_unique<CounterRateBuffer>()} {
    // m_geopos_manager.set_lockin_ready_callback(std::bind(&Daemon::onGeoPosLockInReady, this,
    // std::placeholders::_1));
    // m_geopos_manager.set_valid_pos_callback(std::bind(&Daemon::onGeoPosValid, this,
    // std::placeholders::_1)); m_geopos_manager.set_mode_config(config.position_mode_config);
}

DataStore::~DataStore() {
}

auto DataStore::histo(const std::string& histoName) -> std::shared_ptr<Histogram> {
    auto it = m_histo_map.find(histoName);
    if (it != m_histo_map.end()) {
        return it->second;
    }
    return nullptr;
}

auto DataStore::clearHisto(const std::string& histoName) -> std::shared_ptr<Histogram> {
    auto it = m_histo_map.find(histoName);
    if (it != m_histo_map.end()) {
        it->second->clear();
        return it->second;
    }
    return nullptr;
}

auto DataStore::allHistos() const
    -> const std::unordered_map<std::string, std::shared_ptr<Histogram>>& {
    return m_histo_map;
}

void DataStore::setupHistos() {
    m_histo_map.emplace("geoHeight",
                        std::make_shared<Histogram>("geoHeight", 200, 0., 199., true, "m"));
    m_histo_map.emplace("geoLongitude",
                        std::make_shared<Histogram>("geoLongitude", 200, 0., 0.003, true, "deg"));
    m_histo_map.emplace("geoLatitude",
                        std::make_shared<Histogram>("geoLatitude", 200, 0., 0.003, true, "deg"));
    m_histo_map.emplace("weightedGeoHeight",
                        std::make_shared<Histogram>("weightedGeoHeight", 200, 0., 199., true, "m"));
    m_histo_map.emplace("pulseHeight",
                        std::make_shared<Histogram>("pulseHeight", 500, 0., 3.8, false, "V"));
    m_histo_map.emplace("adcSampleTime",
                        std::make_shared<Histogram>("adcSampleTime", 500, 0., 10., true, "ms"));
    m_histo_map.emplace("UbxEventLength",
                        std::make_shared<Histogram>("UbxEventLength", 100, 50., 149., true, "ns"));
    m_histo_map.emplace("gpioEventInterval", std::make_shared<Histogram>("gpioEventInterval", 400,
                                                                         0., 2000., true, "ms"));
    m_histo_map.emplace(
        "gpioEventIntervalShort",
        std::make_shared<Histogram>("gpioEventIntervalShort", 50, 0., 49., false, "us"));
    m_histo_map.emplace("UbxEventInterval", std::make_shared<Histogram>("UbxEventInterval", 200, 0.,
                                                                        2000., true, "ms"));
    m_histo_map.emplace("TPTimePhase", std::make_shared<Histogram>("TPTimePhase", 500, -500000.,
                                                                   500000., false, "us"));
    m_histo_map.emplace("TPTimeJitter",
                        std::make_shared<Histogram>("TPTimeJitter", 400, -50., 50., true, "us"));
    // m_histo_map.emplace(
    //     "Time-to-Digital Time Diff",
    //     std::make_shared<Histogram>("Time-to-Digital Time Diff", 400, 0., 1e6, true, "ns"));
    m_histo_map.emplace("Bias Voltage",
                        std::make_shared<Histogram>("Bias Voltage", 200, 0., 80., true, "V"));
    m_histo_map.emplace("Bias Current",
                        std::make_shared<Histogram>("Bias Current", 200, 0., 50., true, "uA"));
    m_histo_map.emplace("pDOP", std::make_shared<Histogram>("pDOP", 200, 0., 10., true));
    m_histo_map.emplace("tDOP", std::make_shared<Histogram>("tDOP", 200, 0., 10., true));

    m_geopos_manager->set_histos(m_histo_map["geoLongitude"], m_histo_map["geoLatitude"],
                                 m_histo_map["geoHeight"]);
}

template <>
void DataStore::fillHisto(const GnssPosStruct& pos) {
    // auto currentDop = get<UbxDopStruct>(); // do not use get here! this will cause deadlock
    auto* currentDop = getUnlocked<UbxDopStruct>();
    if (currentDop == nullptr) {
        logWarn("Could not read UbxDopStruct in fillHisto<GnssPosStruct>");
        return;
    }

    if (1e-3 * pos.vAcc < 100.) {
        if (m_geopos_manager->get_mode() != PositionModeConfig::Mode::LockIn ||
            currentDop->vDOP / 100. < m_geopos_manager->get_mode_config().lock_in_max_dop) {
            m_histo_map["geoHeight"]->fill(1e-3 * pos.hMSL);

            if (currentDop->vDOP > 0) {
                const double heightWeight = 100. / currentDop->vDOP;
                m_histo_map["weightedGeoHeight"]->fill(1e-3 * pos.hMSL, heightWeight);
            }
        }
    }
    if (1e-3 * pos.hAcc < 100.) {
        if (currentDop->hDOP / 100. < m_geopos_manager->get_mode_config().lock_in_max_dop) {
            m_histo_map["geoLongitude"]->fill(1e-7 * pos.lon);
            m_histo_map["geoLatitude"]->fill(1e-7 * pos.lat);
        }
    }
}

template <>
void DataStore::fillHisto(const UbxTimeMarkStruct& tm) {
    static UbxTimeMarkStruct lastTimeMark{};

    long double dts = (tm.falling.tv_sec - tm.rising.tv_sec) * 1.0e9L;
    dts += (tm.falling.tv_nsec - tm.rising.tv_nsec);
    if ((dts > 0.0L) && tm.fallingValid) {
        m_histo_map["UbxEventLength"]->fill(static_cast<double>(dts));
    }
    long double interval = (tm.rising.tv_sec - lastTimeMark.rising.tv_sec) * 1.0e9L;
    interval += (tm.rising.tv_nsec - lastTimeMark.rising.tv_nsec);
    if (interval < 1e12)
        m_histo_map["UbxEventInterval"]->fill(static_cast<double>(1.0e-6L * interval));
    // uint16_t diffCount = tm.evtCounter - lastTimeMark.evtCounter;
    // emit timeMarkIntervalCountUpdate(diffCount, static_cast<double>(interval * 1.0e-9L));
    lastTimeMark = tm;
}

template <>
void DataStore::fillHisto(const BiasCurrentEvent& event) {
    m_histo_map["Bias Current"]->fill(event.ibias);
}

template <>
void DataStore::fillHisto(const IntervalEvent& event) {
    auto* currentEventTrigger = getUnlocked<EventTriggerEvent>();
    if (currentEventTrigger == nullptr) {
        logWarn("Could not read EventTriggerEvent in fillHisto<IntervalEvent>");
        return;
    }
    if (event.sig == currentEventTrigger->signal) {
        m_histo_map["gpioEventInterval"]->fill(1e-6 * event.interval.count());
        m_histo_map["gpioEventIntervalShort"]->fill(1e-3 * event.interval.count());
    }
}

template <>
void DataStore::fillHisto(const GpioEvent& event) {
    if (event.gpio_signal != GPIO_SIGNAL::TIMEPULSE) {
        return;
    }

    auto usecs =
        std::chrono::duration_cast<std::chrono::microseconds>(event.timestamp.time_since_epoch())
            .count();
    usecs = usecs % 1'000'000LL;
    if (usecs > 500'000LL) {
        usecs -= 1'000'000LL;
    }

    m_histo_map["TPTimePhase"]->fill(static_cast<double>(usecs));

    if (event.edge != EventEdge::Rising) {
        return;
    }

    if (m_last_timepulse_rising.has_value()) {
        const auto interval = event.timestamp - *m_last_timepulse_rising;
        const double intervalUs = std::chrono::duration<double, std::micro>(interval).count();
        const double jitterUs = intervalUs - 1.0e6;
        if (std::abs(jitterUs) < 5.0e5) {
            m_histo_map["TPTimeJitter"]->fill(jitterUs);
        }
    }
    m_last_timepulse_rising = event.timestamp;
}

template <>
void DataStore::fillHisto(const ADS1115Event& event) {
    if (event.channel == MuonPi::Config::Hardware::ADC::Channel::amplitude) {
        m_histo_map["pulseHeight"]->fill(event.voltage);
    }
    if (event.channel == MuonPi::Config::Hardware::ADC::Channel::bias2) {
        auto* raw = getUnlocked<std::weak_ptr<ShowerDetectorCalib>>();
        if (raw != nullptr) {
            if (auto calib = raw->lock()) {
                const auto& vdivItem = calib->getCalibItem("VDIV");
                if (vdivItem.name == "VDIV") {
                    double vdiv{0.};
                    ShowerDetectorCalib::getValueFromString(vdivItem.value, vdiv);
                    m_histo_map["Bias Voltage"]->fill(event.voltage * vdiv * 0.01);
                }
            }
        }
    }
    m_histo_map["adcSampleTime"]->fill(event.convTime);
}

template <>
void DataStore::fillHisto(const UbxDopStruct& event) {
    m_histo_map["pDOP"]->fill(event.pDOP / 100.);
    m_histo_map["tDOP"]->fill(event.tDOP / 100.);
}

auto DataStore::geoPosManager() -> GeoPosManager& {
    return *m_geopos_manager;
}

auto DataStore::ubloxRateBuffer() -> CounterRateBuffer& {
    return *m_ublox_ratebuffer;
}
