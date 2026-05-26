#include "core/registries/data_store.h"

#include "core/logging/logger.h"
#include "data/events/bias_current_event.h"
#include "data/events/ubx_event.h"
#include "data/histogram.h"
#include "data/ublox/ublox_messages.h"
#include "utility/geoposmanager.h"
#include "utility/ublox_ratebuffer.h"
// #include "data/"

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
    m_histo_map.emplace("TPTimeDiff",
                        std::make_shared<Histogram>("TPTimeDiff", 200, -999., 1000., true, "us"));
    m_histo_map.emplace(
        "Time-to-Digital Time Diff",
        std::make_shared<Histogram>("Time-to-Digital Time Diff", 400, 0., 1e6, true, "ns"));
    m_histo_map.emplace("Bias Voltage",
                        std::make_shared<Histogram>("Bias Voltage", 200, 0., 1., true, "V"));
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

auto DataStore::geoPosManager() -> GeoPosManager& {
    return *m_geopos_manager;
}

auto DataStore::ubloxRateBuffer() -> CounterRateBuffer& {
    return *m_ublox_ratebuffer;
}