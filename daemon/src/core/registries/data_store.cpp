#include "core/registries/data_store.h"

#include "data/histogram.h"
// #include "data/"

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

    // m_geopos_manager.set_histos(
    //     m_histo_map["geoLongitude"],
    //     m_histo_map["geoLatitude"],
    //     m_histo_map["geoHeight"]);
}