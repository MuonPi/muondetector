#include "capnp/capnp_codec.h"

#include "adc_trace_event.capnp.h"
#include "calib_event.capnp.h"
#include "cfg_gnss.capnp.h"
#include "data/commands/adc_mode_request_cmd.h"
#include "data/commands/adc_sample_trigger_cmd.h"
#include "data/commands/bias_switch_cmd.h"
#include "data/commands/bias_voltage_cmd.h"
#include "data/commands/burst_sampling_cmd.h"
#include "data/commands/calibration_cmd.h"
#include "data/commands/calibration_save_cmd.h"
#include "data/commands/dac_eeprom_set_cmd.h"
#include "data/commands/dac_setting_request_cmd.h"
#include "data/commands/event_trigger_cmd.h"
#include "data/commands/gain_switch_cmd.h"
#include "data/commands/gpio_rate_request_cmd.h"
#include "data/commands/gpio_rate_reset_cmd.h"
#include "data/commands/histogram_clear_cmd.h"
#include "data/commands/histogram_request_cmd.h"
#include "data/commands/i2c_scan_bus_cmd.h"
#include "data/commands/i2c_stats_request_cmd.h"
#include "data/commands/mqtt_inhibit_cmd.h"
#include "data/commands/pca_switch_cmd.h"
#include "data/commands/polarity_switch_cmd.h"
#include "data/commands/preamp_switch_cmd.h"
#include "data/commands/spi_stats_request_cmd.h"
#include "data/commands/temperature_request_cmd.h"
#include "data/commands/threshold_setting_cmd.h"
#include "data/commands/ubx_config_default_cmd.h"
#include "data/commands/ubx_gnss_config_cmd.h"
#include "data/commands/ubx_min_cno_cmd.h"
#include "data/commands/ubx_min_max_sv_cmd.h"
#include "data/commands/ubx_msg_poll_cmd.h"
#include "data/commands/ubx_msg_rate_cmd.h"
#include "data/commands/ubx_rate_cmd.h"
#include "data/commands/ubx_reset_cmd.h"
#include "data/commands/ubx_save_cmd.h"
#include "data/commands/ubx_set_aop_cmd.h"
#include "data/events/adc_mode_event.h"
#include "data/events/adc_trace_event.h"
#include "data/events/ads1115_event.h"
#include "data/events/bias_switch_event.h"
#include "data/events/bias_voltage_event.h"
#include "data/events/calib_event.h"
#include "data/events/event_trigger_event.h"
#include "data/events/gain_switch_event.h"
#include "data/events/gpio_event.h"
#include "data/events/gpio_inhibit_event.h"
#include "data/events/gpio_rate_event.h"
#include "data/events/i2c_stats_event.h"
#include "data/events/mcp4728_event.h"
#include "data/events/mqtt_inhibit_event.h"
#include "data/events/mqtt_status_event.h"
#include "data/events/pca_switch_event.h"
#include "data/events/polarity_switch_event.h"
#include "data/events/preamp_switch_event.h"
#include "data/events/sds011_event.h"
#include "data/events/spi_stats_event.h"
#include "data/events/temperature_event.h"
#include "data/events/threshold_setting_event.h"
#include "data/events/ubx_event.h"
#include "data/events/version_event.h"
#include "data/muondetector_structs.h"
#include "gpio_rate_event.capnp.h"
#include "histogram.capnp.h"
#include "i2c_stats_event.capnp.h"
#include "mcp4728_event.capnp.h"
#include "mon_rx.capnp.h"
#include "mon_tx.capnp.h"
#include "nav_sat.capnp.h"
#include "network/tcpmessage_keys.h"
#include "position_mode_config.capnp.h"
#include "protocol.capnp.h"
#include "ubx_timemark_struct.capnp.h"
#include "version_event.capnp.h"

#include <algorithm>
#include <array>
#include <capnp/serialize.h>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <utility>
#include <vector>

inline capnp::FlatArrayMessageReader makeReader(const std::vector<std::uint8_t>& data) {
    auto wordPtr = reinterpret_cast<const capnp::word*>(data.data());
    auto wordCount = data.size() / sizeof(capnp::word);

    return capnp::FlatArrayMessageReader(kj::ArrayPtr<const capnp::word>(wordPtr, wordCount));
}

auto CapnpCodec<Sds011Event>::encode(const Sds011Event& event) -> std::vector<std::uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<Sds011EventCapnp>();

    root.setId(event.id);
    root.setPm2dot5(event.pm2dot5);
    root.setPm10dot0(event.pm10dot0);

    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();

    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}

auto CapnpCodec<Sds011Event>::decode(const std::vector<std::uint8_t>& data) -> Sds011Event {
    auto reader = makeReader(data);
    auto root = reader.getRoot<Sds011EventCapnp>();
    return Sds011Event{
        .id = root.getId(), .pm2dot5 = root.getPm2dot5(), .pm10dot0 = root.getPm10dot0()};
}

auto CapnpCodec<Sds011Event>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_SDS011_SAMPLE);
}

auto CapnpCodec<ADS1115Event>::encode(const ADS1115Event& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<Ads1115EventCapnp>();

    root.setDeviceId(event.deviceId);
    root.setChannel(event.channel);
    root.setRawValue(event.rawValue);
    root.setVoltage(event.voltage);
    root.setTimestamp(event.timestamp);

    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();

    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}

auto CapnpCodec<ADS1115Event>::decode(const std::vector<std::uint8_t>& data) -> ADS1115Event {
    auto reader = makeReader(data);
    auto root = reader.getRoot<Ads1115EventCapnp>();

    ADS1115Event event;
    event.deviceId = root.getDeviceId();
    event.channel = root.getChannel();
    event.rawValue = root.getRawValue();
    event.voltage = root.getVoltage();
    event.timestamp = root.getTimestamp();

    return event;
}

auto CapnpCodec<ADS1115Event>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_ADC_SAMPLE);
}

auto CapnpCodec<NavSat>::encode(const NavSat& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<NavSatCapnp>();

    root.setITOW(event.iTOW);
    root.setHasVersion(event.version.has_value());
    if (event.version.has_value()) {
        root.setVersion(event.version.value());
    }
    root.setHasGlobFlags(event.globFlags.has_value());
    if (event.globFlags.has_value()) {
        root.setGlobFlags(event.globFlags.value());
    }
    root.setNumSvs(event.numSvs);
    root.setGoodSats(event.goodSats);

    auto satellites = root.initSatellites(static_cast<capnp::uint>(event.satellites.size()));
    for (capnp::uint i = 0, size = satellites.size(); i < size; ++i) {
        const auto& satellite = event.satellites[i];
        auto satelliteRoot = satellites[i];
        satelliteRoot.setGnssId(satellite.GnssId);
        satelliteRoot.setSatId(satellite.SatId);
        satelliteRoot.setCnr(satellite.Cnr);
        satelliteRoot.setElev(satellite.Elev);
        satelliteRoot.setAzim(satellite.Azim);
        satelliteRoot.setPrRes(satellite.PrRes);
        satelliteRoot.setQuality(satellite.Quality);
        satelliteRoot.setHealth(satellite.Health);
        satelliteRoot.setOrbitSource(satellite.OrbitSource);
        satelliteRoot.setUsed(satellite.Used);
        satelliteRoot.setDiffCorr(satellite.DiffCorr);
        satelliteRoot.setSmoothed(satellite.Smoothed);
    }

    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();

    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}

auto CapnpCodec<NavSat>::decode(const std::vector<std::uint8_t>& data) -> NavSat {
    auto reader = makeReader(data);
    auto root = reader.getRoot<NavSatCapnp>();

    NavSat event;

    event.iTOW = root.getITOW();

    if (root.getHasVersion()) {
        event.version = root.getVersion();
    } else {
        event.version = std::nullopt;
    }

    if (root.getHasGlobFlags()) {
        event.globFlags = root.getGlobFlags();
    } else {
        event.globFlags = std::nullopt;
    }

    event.numSvs = root.getNumSvs();
    event.goodSats = root.getGoodSats();

    auto list = root.getSatellites();
    event.satellites.reserve(list.size());
    for (auto item : list) {
        GnssSatellite s{};
        s.GnssId = item.getGnssId();
        s.SatId = item.getSatId();
        s.Cnr = item.getCnr();
        s.Elev = item.getElev();
        s.Azim = item.getAzim();
        s.PrRes = item.getPrRes();
        s.Quality = item.getQuality();
        s.Health = item.getHealth();
        s.OrbitSource = item.getOrbitSource();
        s.Used = item.getUsed();
        s.DiffCorr = item.getDiffCorr();
        s.Smoothed = item.getSmoothed();
        event.satellites.push_back(std::move(s));
    }
    return event;
}

auto CapnpCodec<NavSat>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GNSS_SATS);
}

auto CapnpCodec<GpioEvent>::encode(const GpioEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GpioEventCapnp>();

    root.setSig(static_cast<std::uint8_t>(event.gpio_signal));
    root.setPin(static_cast<std::uint8_t>(event.gpio_pin));
    root.setTimestamp(static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(event.timestamp.time_since_epoch())
            .count()));
    root.setEdge(static_cast<std::uint8_t>(event.edge));

    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();

    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}

auto CapnpCodec<GpioEvent>::decode(const std::vector<std::uint8_t>& data) -> GpioEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GpioEventCapnp>();

    GpioEvent event;

    event.gpio_signal = static_cast<GPIO_SIGNAL>(root.getSig());
    event.gpio_pin = static_cast<unsigned int>(root.getPin());

    event.timestamp =
        std::chrono::steady_clock::time_point(std::chrono::nanoseconds(root.getTimestamp()));

    event.edge = static_cast<EventEdge>(root.getEdge());

    return event;
}

auto CapnpCodec<GpioEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GPIO_EVENT);
}

auto CapnpCodec<AdcTraceEvent>::encode(const AdcTraceEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<AdcTraceEventCapnp>();
    auto samples = root.initAdcSampleBuffer(event.adcSampleBuffer.size());
    for (std::size_t i = 0; i < event.adcSampleBuffer.size(); ++i) {
        samples.set(i, event.adcSampleBuffer[i]);
    }
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<AdcTraceEvent>::decode(const std::vector<std::uint8_t>& data) -> AdcTraceEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<AdcTraceEventCapnp>();
    AdcTraceEvent event;
    auto samples = root.getAdcSampleBuffer();
    event.adcSampleBuffer.reserve(samples.size());
    for (auto value : samples) {
        event.adcSampleBuffer.push_back(value);
    }
    return event;
}

auto CapnpCodec<AdcTraceEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_ADC_TRACE);
}

auto CapnpCodec<CalibEvent>::encode(const CalibEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<CalibEventCapnp>();
    root.setValid(event.valid);
    root.setEepromValid(event.eepromValid);
    root.setId(event.id);
    auto list = root.initCalibList(event.calibList.size());
    for (std::size_t i = 0; i < event.calibList.size(); ++i) {
        auto item = list[i];
        item.setName(event.calibList[i].name);
        item.setType(event.calibList[i].type);
        item.setAddress(event.calibList[i].address);
        item.setValue(event.calibList[i].value);
    }
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<CalibEvent>::decode(const std::vector<std::uint8_t>& data) -> CalibEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<CalibEventCapnp>();
    CalibEvent event;
    event.valid = root.getValid();
    event.eepromValid = root.getEepromValid();
    event.id = root.getId();
    auto list = root.getCalibList();
    event.calibList.reserve(list.size());
    for (auto item : list) {
        CalibStruct c{};
        c.name = item.getName().cStr();
        c.type = item.getType().cStr();
        c.address = item.getAddress();
        c.value = item.getValue().cStr();
        event.calibList.push_back(c);
    }
    return event;
}

auto CapnpCodec<CalibEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_CALIB_SET);
}

auto CapnpCodec<CfgGNSS>::encode(const CfgGNSS& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<CfgGNSSCapnp>();
    root.setVersion(event.version);
    root.setNumTrkChHw(event.numTrkChHw);
    root.setNumTrkChUse(event.numTrkChUse);
    root.setNumConfigBlocks(event.numConfigBlocks);
    auto cfgs = root.initConfigs(event.configs.size());
    for (std::size_t i = 0; i < event.configs.size(); ++i) {
        auto cfg = cfgs[i];
        cfg.setGnssId(event.configs[i].gnssId);
        cfg.setResTrkCh(event.configs[i].resTrkCh);
        cfg.setMaxTrkCh(event.configs[i].maxTrkCh);
        cfg.setFlags(event.configs[i].flags);
    }
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<CfgGNSS>::decode(const std::vector<std::uint8_t>& data) -> CfgGNSS {
    auto reader = makeReader(data);
    auto root = reader.getRoot<CfgGNSSCapnp>();
    CfgGNSS event{};
    event.version = root.getVersion();
    event.numTrkChHw = root.getNumTrkChHw();
    event.numTrkChUse = root.getNumTrkChUse();
    event.numConfigBlocks = root.getNumConfigBlocks();
    auto cfgs = root.getConfigs();
    event.configs.reserve(cfgs.size());
    for (auto cfg : cfgs) {
        event.configs.push_back(GnssConfigStruct{cfg.getGnssId(), cfg.getResTrkCh(),
                                                 cfg.getMaxTrkCh(), cfg.getFlags()});
    }
    return event;
}
auto CapnpCodec<CfgGNSS>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_GNSS_CONFIG);
}

auto CapnpCodec<GpioRateEvent>::encode(const GpioRateEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GpioRateEventCapnp>();
    root.setWhichRate(event.whichRate);
    auto rates = root.initRate(event.rate.size());
    for (std::size_t i = 0; i < event.rate.size(); ++i) {
        rates[i].setFirst(event.rate[i].first);
        rates[i].setSecond(event.rate[i].second);
    }
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<GpioRateEvent>::decode(const std::vector<std::uint8_t>& data) -> GpioRateEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GpioRateEventCapnp>();
    GpioRateEvent event{};
    event.whichRate = root.getWhichRate();
    auto rates = root.getRate();
    event.rate.reserve(rates.size());
    for (auto r : rates) {
        event.rate.emplace_back(r.getFirst(), r.getSecond());
    }
    return event;
}

auto CapnpCodec<GpioRateEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GPIO_RATE);
}

auto CapnpCodec<I2CStatsEvent>::encode(const I2CStatsEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<I2cStatsEventCapnp>();
    root.setNrDevices(event.nrDevices);
    root.setBytesRead(event.bytesRead);
    root.setBytesWritten(event.bytesWritten);
    auto devices = root.initDeviceList(event.deviceList.size());
    for (std::size_t i = 0; i < event.deviceList.size(); ++i) {
        auto d = devices[i];
        d.setAddress(event.deviceList[i].address);
        d.setName(event.deviceList[i].name);
        d.setStatus(event.deviceList[i].status);
        d.setNrBytesWritten(event.deviceList[i].nrBytesWritten);
        d.setNrBytesRead(event.deviceList[i].nrBytesRead);
        d.setNrIoErrors(event.deviceList[i].nrIoErrors);
        d.setLastTransactionTime(event.deviceList[i].lastTransactionTime);
    }
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<I2CStatsEvent>::decode(const std::vector<std::uint8_t>& data) -> I2CStatsEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<I2cStatsEventCapnp>();
    I2CStatsEvent event{};
    event.nrDevices = root.getNrDevices();
    event.bytesRead = root.getBytesRead();
    event.bytesWritten = root.getBytesWritten();
    auto devices = root.getDeviceList();
    event.deviceList.reserve(devices.size());
    for (auto d : devices) {
        event.deviceList.push_back(I2cDeviceEntry{d.getAddress(), d.getName().cStr(), d.getStatus(),
                                                  d.getNrBytesWritten(), d.getNrBytesRead(),
                                                  d.getNrIoErrors(), d.getLastTransactionTime()});
    }
    return event;
}

auto CapnpCodec<I2CStatsEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_I2C_STATS);
}

auto CapnpCodec<MCP4728Event>::encode(const MCP4728Event& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<MCP4728EventCapnp>();
    auto dacValues = root.initDacValues(event.dacValues.size());
    std::size_t i = 0;
    for (const auto& [channel, value] : event.dacValues) {
        dacValues[i].setChannel(channel);
        dacValues[i].setValue(value);
        ++i;
    }
    root.setHasEepromValue(event.eepromValues.has_value());
    if (event.eepromValues.has_value()) {
        auto eepromValues = root.initEepromValues(event.eepromValues.value().size());
        i = 0;
        for (const auto& [channel, value] : event.eepromValues.value()) {
            eepromValues[i].setChannel(channel);
            eepromValues[i].setValue(value);
            ++i;
        }
    }
    auto voltages = root.initVoltages(event.voltages.size());
    i = 0;
    for (const auto& [channel, value] : event.voltages) {
        voltages[i].setChannel(channel);
        voltages[i].setValue(value);
        ++i;
    }
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<MCP4728Event>::decode(const std::vector<std::uint8_t>& data) -> MCP4728Event {
    auto reader = makeReader(data);
    auto root = reader.getRoot<MCP4728EventCapnp>();
    MCP4728Event event{};
    for (auto entry : root.getDacValues()) {
        event.dacValues[entry.getChannel()] = entry.getValue();
    }
    if (root.getHasEepromValue()) {
        std::unordered_map<std::uint8_t, std::uint16_t> eepromValues{};
        for (auto entry : root.getEepromValues()) {
            eepromValues[entry.getChannel()] = entry.getValue();
        }
        event.eepromValues.emplace(std::move(eepromValues));
    }
    for (auto entry : root.getVoltages()) {
        event.voltages[entry.getChannel()] = entry.getValue();
    }
    return event;
}

auto CapnpCodec<MCP4728Event>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_DAC_READBACK);
}

template <typename TCapnp, typename TEvent>
auto encodeMonBuffer(const TEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<TCapnp>();
    auto pending = root.initPending(event.pending.size());
    auto usage = root.initUsage(event.usage.size());
    auto peakUsage = root.initPeakUsage(event.peakUsage.size());
    for (std::size_t i = 0; i < event.pending.size(); ++i) {
        pending.set(i, event.pending[i]);
    }
    for (std::size_t i = 0; i < event.usage.size(); ++i) {
        usage.set(i, event.usage[i]);
    }
    for (std::size_t i = 0; i < event.peakUsage.size(); ++i) {
        peakUsage.set(i, event.peakUsage[i]);
    }
    root.setTUsage(event.tUsage);
    root.setTPeakUsage(event.tPeakUsage);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

template <typename TCapnp, typename TEvent>
auto decodeMonBuffer(const std::vector<std::uint8_t>& data) -> TEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<TCapnp>();
    TEvent event{};
    auto pending = root.getPending();
    auto usage = root.getUsage();
    auto peakUsage = root.getPeakUsage();
    for (std::size_t i = 0;
         i < std::min<std::size_t>(event.pending.size(), static_cast<std::size_t>(pending.size()));
         ++i) {
        event.pending[i] = pending[i];
    }
    for (std::size_t i = 0;
         i < std::min<std::size_t>(event.usage.size(), static_cast<std::size_t>(usage.size()));
         ++i) {
        event.usage[i] = usage[i];
    }
    for (std::size_t i = 0; i < std::min<std::size_t>(event.peakUsage.size(),
                                                      static_cast<std::size_t>(peakUsage.size()));
         ++i) {
        event.peakUsage[i] = peakUsage[i];
    }
    event.tUsage = root.getTUsage();
    event.tPeakUsage = root.getTPeakUsage();
    return event;
}

auto CapnpCodec<MonRx>::encode(const MonRx& event) -> std::vector<uint8_t> {
    return encodeMonBuffer<MonRxCapnp>(event);
}

auto CapnpCodec<MonRx>::decode(const std::vector<std::uint8_t>& data) -> MonRx {
    return decodeMonBuffer<MonRxCapnp, MonRx>(data);
}

auto CapnpCodec<MonRx>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_RXBUF);
}

auto CapnpCodec<MonTx>::encode(const MonTx& event) -> std::vector<uint8_t> {
    return encodeMonBuffer<MonTxCapnp>(event);
}

auto CapnpCodec<MonTx>::decode(const std::vector<std::uint8_t>& data) -> MonTx {
    return decodeMonBuffer<MonTxCapnp, MonTx>(data);
}

auto CapnpCodec<MonTx>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_TXBUF);
}

auto CapnpCodec<PositionModeConfig>::encode(const PositionModeConfig& event)
    -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<PositionModeConfigCapnp>();
    root.setMode(static_cast<PositionModeConfigCapnp::Mode>(event.mode));
    auto pos = root.initStaticPosition();
    pos.setLongitude(event.static_position.longitude);
    pos.setLatitude(event.static_position.latitude);
    pos.setAltitude(event.static_position.altitude);
    pos.setHorError(event.static_position.hor_error);
    pos.setVertError(event.static_position.vert_error);
    root.setLockInMaxDop(event.lock_in_max_dop);
    root.setLockInMinErrorMeters(event.lock_in_min_error_meters);
    root.setFilterConfig(static_cast<PositionModeConfigCapnp::FilterType>(event.filter_config));
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<PositionModeConfig>::decode(const std::vector<std::uint8_t>& data)
    -> PositionModeConfig {
    auto reader = makeReader(data);
    auto root = reader.getRoot<PositionModeConfigCapnp>();
    PositionModeConfig event{};
    event.mode = static_cast<PositionModeConfig::Mode>(root.getMode());
    auto pos = root.getStaticPosition();
    event.static_position.longitude = pos.getLongitude();
    event.static_position.latitude = pos.getLatitude();
    event.static_position.altitude = pos.getAltitude();
    event.static_position.hor_error = pos.getHorError();
    event.static_position.vert_error = pos.getVertError();
    event.lock_in_max_dop = root.getLockInMaxDop();
    event.lock_in_min_error_meters = root.getLockInMinErrorMeters();
    event.filter_config = static_cast<PositionModeConfig::FilterType>(root.getFilterConfig());
    return event;
}

auto CapnpCodec<PositionModeConfig>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_POSITION_MODEL);
}

auto CapnpCodec<CfgMsg>::encode(const CfgMsg& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<CfgMsgCapnp>();
    root.setMsgID(event.msgID);
    root.setRate(event.rate);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<CfgMsg>::decode(const std::vector<std::uint8_t>& data) -> CfgMsg {
    auto reader = makeReader(data);
    auto root = reader.getRoot<CfgMsgCapnp>();
    return CfgMsg{.msgID = root.getMsgID(), .rate = root.getRate()};
}

auto CapnpCodec<CfgMsg>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_MSG_RATE);
}

auto CapnpCodec<UbxTimeMarkStruct>::encode(const UbxTimeMarkStruct& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<UbxTimeMarkStructCapnp>();
    auto rising = root.initRising();
    rising.setSeconds(event.rising.tv_sec);
    rising.setNanoseconds(event.rising.tv_nsec);
    auto falling = root.initFalling();
    falling.setSeconds(event.falling.tv_sec);
    falling.setNanoseconds(event.falling.tv_nsec);
    root.setRisingValid(event.risingValid);
    root.setFallingValid(event.fallingValid);
    root.setAccuracyNs(event.accuracy_ns);
    root.setValid(event.valid);
    root.setTimeBase(event.timeBase);
    root.setUtcAvailable(event.utcAvailable);
    root.setFlags(event.flags);
    root.setEvtCounter(event.evtCounter);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<UbxTimeMarkStruct>::decode(const std::vector<std::uint8_t>& data)
    -> UbxTimeMarkStruct {
    auto reader = makeReader(data);
    auto root = reader.getRoot<UbxTimeMarkStructCapnp>();
    UbxTimeMarkStruct event{};
    auto rising = root.getRising();
    event.rising.tv_sec = rising.getSeconds();
    event.rising.tv_nsec = rising.getNanoseconds();
    auto falling = root.getFalling();
    event.falling.tv_sec = falling.getSeconds();
    event.falling.tv_nsec = falling.getNanoseconds();
    event.risingValid = root.getRisingValid();
    event.fallingValid = root.getFallingValid();
    event.accuracy_ns = root.getAccuracyNs();
    event.valid = root.getValid();
    event.timeBase = root.getTimeBase();
    event.utcAvailable = root.getUtcAvailable();
    event.flags = root.getFlags();
    event.evtCounter = root.getEvtCounter();
    return event;
}

auto CapnpCodec<UbxTimeMarkStruct>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_TIMEMARK);
}

auto CapnpCodec<VersionEvent>::encode(const VersionEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<VersionEventCapnp>();
    auto hw = root.initHwVer();
    hw.setMajor(event.hw_ver.major);
    hw.setMinor(event.hw_ver.minor);
    hw.setPatch(event.hw_ver.patch);
    hw.setAdditional(event.hw_ver.additional);
    hw.setHash(event.hw_ver.hash);
    auto sw = root.initSwVer();
    sw.setMajor(event.sw_ver.major);
    sw.setMinor(event.sw_ver.minor);
    sw.setPatch(event.sw_ver.patch);
    sw.setAdditional(event.sw_ver.additional);
    sw.setHash(event.sw_ver.hash);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<VersionEvent>::decode(const std::vector<std::uint8_t>& data) -> VersionEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<VersionEventCapnp>();
    VersionEvent event{};
    auto hw = root.getHwVer();
    event.hw_ver.major = hw.getMajor();
    event.hw_ver.minor = hw.getMinor();
    event.hw_ver.patch = hw.getPatch();
    event.hw_ver.additional = hw.getAdditional().cStr();
    event.hw_ver.hash = hw.getHash().cStr();
    auto sw = root.getSwVer();
    event.sw_ver.major = sw.getMajor();
    event.sw_ver.minor = sw.getMinor();
    event.sw_ver.patch = sw.getPatch();
    event.sw_ver.additional = sw.getAdditional().cStr();
    event.sw_ver.hash = sw.getHash().cStr();
    return event;
}

auto CapnpCodec<VersionEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_VERSION);
}

auto CapnpCodec<AdcModeEvent>::encode(const AdcModeEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<AdcModeEventCapnp>();
    root.setMode(event.mode);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<AdcModeEvent>::decode(const std::vector<std::uint8_t>& data) -> AdcModeEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<AdcModeEventCapnp>();
    AdcModeEvent event{};
    event.mode = root.getMode();
    return event;
}
auto CapnpCodec<AdcModeEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_ADC_MODE);
}

auto CapnpCodec<BiasSwitchEvent>::encode(const BiasSwitchEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<BiasSwitchEventCapnp>();
    root.setBiasOn(event.biasOn);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<BiasSwitchEvent>::decode(const std::vector<std::uint8_t>& data) -> BiasSwitchEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<BiasSwitchEventCapnp>();
    BiasSwitchEvent event{};
    event.biasOn = root.getBiasOn();
    return event;
}
auto CapnpCodec<BiasSwitchEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_BIAS_SWITCH);
}

auto CapnpCodec<BiasVoltageEvent>::encode(const BiasVoltageEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<BiasVoltageEventCapnp>();
    root.setVoltage(event.voltage);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<BiasVoltageEvent>::decode(const std::vector<std::uint8_t>& data)
    -> BiasVoltageEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<BiasVoltageEventCapnp>();
    BiasVoltageEvent event{};
    event.voltage = root.getVoltage();
    return event;
}
auto CapnpCodec<BiasVoltageEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_BIAS_VOLTAGE);
}

auto CapnpCodec<GainSwitchEvent>::encode(const GainSwitchEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GainSwitchEventCapnp>();
    root.setState(event.state);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<GainSwitchEvent>::decode(const std::vector<std::uint8_t>& data) -> GainSwitchEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GainSwitchEventCapnp>();
    GainSwitchEvent event{};
    event.state = root.getState();
    return event;
}
auto CapnpCodec<GainSwitchEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GAIN_SWITCH);
}

auto CapnpCodec<PcaSwitchEvent>::encode(const PcaSwitchEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<PcaSwitchEventCapnp>();
    root.setPcaPortMask(event.pcaPortMask);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<PcaSwitchEvent>::decode(const std::vector<std::uint8_t>& data) -> PcaSwitchEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<PcaSwitchEventCapnp>();
    PcaSwitchEvent event{};
    event.pcaPortMask = root.getPcaPortMask();
    return event;
}
auto CapnpCodec<PcaSwitchEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PCA_SWITCH);
}

auto CapnpCodec<PolaritySwitchEvent>::encode(const PolaritySwitchEvent& event)
    -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<PolaritySwitchEventCapnp>();
    root.setPol1(event.pol1);
    root.setPol2(event.pol2);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<PolaritySwitchEvent>::decode(const std::vector<std::uint8_t>& data)
    -> PolaritySwitchEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<PolaritySwitchEventCapnp>();
    return {root.getPol1(), root.getPol2()};
}
auto CapnpCodec<PolaritySwitchEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_POLARITY_SWITCH);
}

auto CapnpCodec<PreampSwitchEvent>::encode(const PreampSwitchEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<PreampSwitchEventCapnp>();
    root.setChannel(event.channel);
    root.setState(event.state);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<PreampSwitchEvent>::decode(const std::vector<std::uint8_t>& data)
    -> PreampSwitchEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<PreampSwitchEventCapnp>();
    return {root.getChannel(), root.getState()};
}
auto CapnpCodec<PreampSwitchEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PREAMP_SWITCH);
}

auto CapnpCodec<GpioInhibitEvent>::encode(const GpioInhibitEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GpioInhibitEventCapnp>();
    root.setInhibit(event.inhibit);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<GpioInhibitEvent>::decode(const std::vector<std::uint8_t>& data)
    -> GpioInhibitEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GpioInhibitEventCapnp>();
    GpioInhibitEvent event{};
    event.inhibit = root.getInhibit();
    return event;
}
auto CapnpCodec<GpioInhibitEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GPIO_INHIBIT);
}

auto CapnpCodec<MqttInhibitEvent>::encode(const MqttInhibitEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<MqttInhibitEventCapnp>();
    root.setInhibit(event.inhibit);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<MqttInhibitEvent>::decode(const std::vector<std::uint8_t>& data)
    -> MqttInhibitEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<MqttInhibitEventCapnp>();
    MqttInhibitEvent event{};
    event.inhibit = root.getInhibit();
    return event;
}
auto CapnpCodec<MqttInhibitEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_MQTT_INHIBIT);
}

auto CapnpCodec<TemperatureEvent>::encode(const TemperatureEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<LM75EventCapnp>();
    root.setTemperature(event.temperature);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<TemperatureEvent>::decode(const std::vector<std::uint8_t>& data)
    -> TemperatureEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<LM75EventCapnp>();
    TemperatureEvent event{};
    event.temperature = root.getTemperature();
    return event;
}
auto CapnpCodec<TemperatureEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_TEMPERATURE);
}

auto CapnpCodec<MqttStatusEvent>::encode(const MqttStatusEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<MqttStatusEventCapnp>();
    root.setStatus(static_cast<std::uint8_t>(event.status));
    root.setText(event.text);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<MqttStatusEvent>::decode(const std::vector<std::uint8_t>& data) -> MqttStatusEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<MqttStatusEventCapnp>();
    return MqttStatusEvent{.status = static_cast<MqttStatusEvent::Status>(root.getStatus()),
                           .text = root.getText()};
}
auto CapnpCodec<MqttStatusEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_MQTT_STATUS);
}

auto CapnpCodec<SPIStatsEvent>::encode(const SPIStatsEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<SPIStatsEventCapnp>();
    root.setSpiPresent(event.spiPresent);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<SPIStatsEvent>::decode(const std::vector<std::uint8_t>& data) -> SPIStatsEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<SPIStatsEventCapnp>();
    SPIStatsEvent event{};
    event.spiPresent = root.getSpiPresent();
    return event;
}
auto CapnpCodec<SPIStatsEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_SPI_STATS);
}

auto CapnpCodec<ThresholdSettingEvent>::encode(const ThresholdSettingEvent& event)
    -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<ThresholdSettingEventCapnp>();
    root.setChannel(event.channel);
    root.setVoltage(event.voltage);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<ThresholdSettingEvent>::decode(const std::vector<std::uint8_t>& data)
    -> ThresholdSettingEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<ThresholdSettingEventCapnp>();
    return {root.getChannel(), root.getVoltage()};
}
auto CapnpCodec<ThresholdSettingEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_THRESHOLD);
}

auto CapnpCodec<EventTriggerEvent>::encode(const EventTriggerEvent& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<EventTriggerEventCapnp>();
    root.setSignal(static_cast<std::uint8_t>(event.signal));
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<EventTriggerEvent>::decode(const std::vector<std::uint8_t>& data)
    -> EventTriggerEvent {
    auto reader = makeReader(data);
    auto root = reader.getRoot<EventTriggerEventCapnp>();
    return {static_cast<GPIO_SIGNAL>(root.getSignal())};
}
auto CapnpCodec<EventTriggerEvent>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_EVENTTRIGGER);
}

auto CapnpCodec<GnssPosStruct>::encode(const GnssPosStruct& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GnssPosStructCapnp>();
    root.setITOW(event.iTOW);
    root.setLon(event.lon);
    root.setLat(event.lat);
    root.setHeight(event.height);
    root.setHMSL(event.hMSL);
    root.setHAcc(event.hAcc);
    root.setVAcc(event.vAcc);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<GnssPosStruct>::decode(const std::vector<std::uint8_t>& data) -> GnssPosStruct {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GnssPosStructCapnp>();
    return {root.getITOW(), root.getLon(),  root.getLat(), root.getHeight(),
            root.getHMSL(), root.getHAcc(), root.getVAcc()};
}
auto CapnpCodec<GnssPosStruct>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GEO_POS);
}

auto CapnpCodec<GnssMonHwStruct>::encode(const GnssMonHwStruct& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GnssMonHwStructCapnp>();
    root.setNoisePerMS(event.noisePerMS);
    root.setAgcCnt(event.agcCnt);
    root.setAntStatus(event.antStatus);
    root.setAntPower(event.antPower);
    root.setFlags(event.flags);
    root.setJamInd(event.jamInd);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<GnssMonHwStruct>::decode(const std::vector<std::uint8_t>& data) -> GnssMonHwStruct {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GnssMonHwStructCapnp>();
    return {root.getNoisePerMS(), root.getAgcCnt(), root.getAntStatus(),
            root.getAntPower(),   root.getFlags(),  root.getJamInd()};
}
auto CapnpCodec<GnssMonHwStruct>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_MONHW);
}

auto CapnpCodec<GnssMonHw2Struct>::encode(const GnssMonHw2Struct& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GnssMonHw2StructCapnp>();
    root.setOfsI(event.ofsI);
    root.setMagI(event.magI);
    root.setOfsQ(event.ofsQ);
    root.setMagQ(event.magQ);
    root.setCfgSrc(event.cfgSrc);
    root.setPostStatus(event.postStatus);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<GnssMonHw2Struct>::decode(const std::vector<std::uint8_t>& data)
    -> GnssMonHw2Struct {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GnssMonHw2StructCapnp>();
    return {root.getOfsI(), root.getMagI(),   root.getOfsQ(),
            root.getMagQ(), root.getCfgSrc(), root.getPostStatus()};
}
auto CapnpCodec<GnssMonHw2Struct>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_MONHW2);
}

auto CapnpCodec<GpsVersion>::encode(const GpsVersion& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GpsVersionCapnp>();
    root.setHwString(event.hwString);
    root.setSwString(event.swString);
    root.setProt(event.prot);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<GpsVersion>::decode(const std::vector<std::uint8_t>& data) -> GpsVersion {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GpsVersionCapnp>();
    return {root.getHwString().cStr(), root.getSwString().cStr(), root.getProt().cStr()};
}
auto CapnpCodec<GpsVersion>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_VERSION);
}

auto CapnpCodec<NavClock>::encode(const NavClock& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<NavClockCapnp>();
    root.setITOW(event.iTOW);
    root.setClkB(event.clkB);
    root.setClkD(event.clkD);
    root.setTAcc(event.tAcc);
    root.setFAcc(event.fAcc);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<NavClock>::decode(const std::vector<std::uint8_t>& data) -> NavClock {
    auto reader = makeReader(data);
    auto root = reader.getRoot<NavClockCapnp>();
    return NavClock{
        .iTOW = root.getITOW(),
        .clkB = root.getClkB(),
        .clkD = root.getClkD(),
        .tAcc = root.getTAcc(),
        .fAcc = root.getFAcc(),
    };
}
auto CapnpCodec<NavClock>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_NAVCLOCK);
}

auto CapnpCodec<NavStatus>::encode(const NavStatus& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<NavStatusCapnp>();
    root.setGDOP(static_cast<std::uint16_t>(event.iTOW));
    root.setPDOP(event.gpsFix);
    root.setTDOP(event.flags);
    root.setVDOP(event.flags2);
    root.setHDOP(static_cast<std::uint16_t>(event.ttff));
    root.setNDOP(static_cast<std::uint16_t>(event.msss));
    root.setEDOP(0);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<NavStatus>::decode(const std::vector<std::uint8_t>& data) -> NavStatus {
    auto reader = makeReader(data);
    auto root = reader.getRoot<NavStatusCapnp>();
    NavStatus event{};
    event.iTOW = root.getGDOP();
    event.gpsFix = static_cast<std::uint8_t>(root.getPDOP());
    event.flags = static_cast<std::uint8_t>(root.getTDOP());
    event.flags2 = static_cast<std::uint8_t>(root.getVDOP());
    event.ttff = root.getHDOP();
    event.msss = root.getNDOP();
    return event;
}
auto CapnpCodec<NavStatus>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_NAVSTATUS);
}

auto CapnpCodec<UbxTimePulseStruct>::encode(const UbxTimePulseStruct& event)
    -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<UbxTimePulseStructCapnp>();
    root.setTpIndex(event.tpIndex);
    root.setVersion(event.version);
    root.setAntCableDelay(event.antCableDelay);
    root.setRfGroupDelay(event.rfGroupDelay);
    root.setFreqPeriod(event.freqPeriod);
    root.setFreqPeriodLock(event.freqPeriodLock);
    root.setPulseLenRatio(event.pulseLenRatio);
    root.setPulseLenRatioLock(event.pulseLenRatioLock);
    root.setUserConfigDelay(event.userConfigDelay);
    root.setFlags(event.flags);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<UbxTimePulseStruct>::decode(const std::vector<std::uint8_t>& data)
    -> UbxTimePulseStruct {
    auto reader = makeReader(data);
    auto root = reader.getRoot<UbxTimePulseStructCapnp>();
    UbxTimePulseStruct event{};
    event.tpIndex = root.getTpIndex();
    event.version = root.getVersion();
    event.antCableDelay = root.getAntCableDelay();
    event.rfGroupDelay = root.getRfGroupDelay();
    event.freqPeriod = root.getFreqPeriod();
    event.freqPeriodLock = root.getFreqPeriodLock();
    event.pulseLenRatio = root.getPulseLenRatio();
    event.pulseLenRatioLock = root.getPulseLenRatioLock();
    event.userConfigDelay = root.getUserConfigDelay();
    event.flags = root.getFlags();
    return event;
}
auto CapnpCodec<UbxTimePulseStruct>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_CFG_TP5);
}

auto CapnpCodec<LogInfoStruct>::encode(const LogInfoStruct& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<LogInfoStructCapnp>();
    root.setLogFileName(event.logFileName);
    root.setDataFileName(event.dataFileName);
    root.setStatus(event.status);
    root.setLogFileSize(event.logFileSize);
    root.setDataFileSize(event.dataFileSize);
    root.setLogAge(event.logAge.count());
    root.setLogRotationDuration(event.logRotationDuration.count());
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<LogInfoStruct>::decode(const std::vector<std::uint8_t>& data) -> LogInfoStruct {
    auto reader = makeReader(data);
    auto root = reader.getRoot<LogInfoStructCapnp>();
    LogInfoStruct event{};
    event.logFileName = root.getLogFileName().cStr();
    event.dataFileName = root.getDataFileName().cStr();
    event.status = static_cast<LogInfoStruct::status_t>(root.getStatus());
    event.logFileSize = root.getLogFileSize();
    event.dataFileSize = root.getDataFileSize();
    event.logAge = std::chrono::seconds(root.getLogAge());
    event.logRotationDuration = std::chrono::seconds(root.getLogRotationDuration());
    return event;
}
auto CapnpCodec<LogInfoStruct>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_LOG_INFO);
}

auto CapnpCodec<Histogram>::encode(const Histogram& h) -> std::vector<uint8_t> {
    ::capnp::MallocMessageBuilder message;

    auto root = message.initRoot<HistogramCapnp>();

    root.setName(h.getName());
    root.setUnit(h.getUnit());

    root.setNrBins(h.getNrBins());
    root.setMin(h.getMin());
    root.setMax(h.getMax());

    root.setOverflow(h.getOverflow());
    root.setUnderflow(h.getUnderflow());

    root.setAutoscale(h.getAutoscale());

    auto bins = root.initBins(h.histogramMap().size());

    std::size_t idx = 0;
    for (const auto& [bin, content] : h.histogramMap()) {
        auto entry = bins[idx++];
        entry.setBin(bin);
        entry.setContent(content);
    }

    kj::VectorOutputStream out;
    capnp::writeMessage(out, message);

    auto arr = out.getArray();
    return std::vector<uint8_t>(arr.begin(), arr.end());
}

auto CapnpCodec<Histogram>::decode(const std::vector<uint8_t>& data) -> Histogram {
    kj::ArrayPtr<const capnp::word> words(reinterpret_cast<const capnp::word*>(data.data()),
                                          data.size() / sizeof(capnp::word));

    capnp::FlatArrayMessageReader reader(words);

    auto root = reader.getRoot<HistogramCapnp>();

    Histogram h;

    h.setName(root.getName().cStr());
    h.setUnit(root.getUnit().cStr());

    h.setNrBins(root.getNrBins());
    h.setMin(root.getMin());
    h.setMax(root.getMax());

    h.setAutoscale(root.getAutoscale());

    for (auto entry : root.getBins()) {
        h.setBinContent(entry.getBin(), entry.getContent());
    }

    return h;
}

auto CapnpCodec<Histogram>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_HISTOGRAM);
}

auto CapnpCodec<StartBurstSampling>::encode(const StartBurstSampling& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<StartBurstSamplingCapnp>();
    root.setFrequencyHz(static_cast<std::uint32_t>(cmd.frequencyHz));
    root.setSamples(static_cast<std::uint32_t>(cmd.samples));
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<StartBurstSampling>::decode(const std::vector<std::uint8_t>& data)
    -> StartBurstSampling {
    auto reader = makeReader(data);
    auto root = reader.getRoot<StartBurstSamplingCapnp>();
    return StartBurstSampling{root.getFrequencyHz(), root.getSamples()};
}

auto CapnpCodec<StartBurstSampling>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_RATE_SCAN);
}

auto CapnpCodec<GainSwitchCmd>::encode(const GainSwitchCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GainSwitchCmdCapnp>();
    root.setState(cmd.state);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<GainSwitchCmd>::decode(const std::vector<std::uint8_t>& data) -> GainSwitchCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GainSwitchCmdCapnp>();
    return GainSwitchCmd{root.getState()};
}
auto CapnpCodec<GainSwitchCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GAIN_SWITCH);
}

auto CapnpCodec<ThresholdSettingCmd>::encode(const ThresholdSettingCmd& cmd)
    -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<DacSettingCmdCapnp>();
    root.setChannel(cmd.channel);
    root.setValue(cmd.value);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<ThresholdSettingCmd>::decode(const std::vector<std::uint8_t>& data)
    -> ThresholdSettingCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<DacSettingCmdCapnp>();
    return ThresholdSettingCmd{root.getChannel(), root.getValue()};
}
auto CapnpCodec<ThresholdSettingCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_THRESHOLD);
}

auto CapnpCodec<PcaSwitchCmd>::encode(const PcaSwitchCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<PcaSwitchCmdCapnp>();
    root.setPcaPortMask(cmd.pcaPortMask);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<PcaSwitchCmd>::decode(const std::vector<std::uint8_t>& data) -> PcaSwitchCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<PcaSwitchCmdCapnp>();
    return PcaSwitchCmd{root.getPcaPortMask()};
}
auto CapnpCodec<PcaSwitchCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PCA_SWITCH);
}

auto CapnpCodec<UbxMsgPollCmd>::encode(const UbxMsgPollCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<UbxMsgPollCmdCapnp>();
    root.setId(static_cast<std::uint16_t>(cmd.id));
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<UbxMsgPollCmd>::decode(const std::vector<std::uint8_t>& data) -> UbxMsgPollCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<UbxMsgPollCmdCapnp>();
    return UbxMsgPollCmd{static_cast<UBX_MSG::msg_id>(root.getId())};
}

auto CapnpCodec<UbxMsgPollCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_MSG_POLL);
}

auto CapnpCodec<UbxMsgRateCmd>::encode(const UbxMsgRateCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<UbxMsgRateCmdCapnp>();
    root.setId(static_cast<std::uint16_t>(cmd.id));
    root.setPort(cmd.port);
    root.setRate(cmd.rate);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<UbxMsgRateCmd>::decode(const std::vector<std::uint8_t>& data) -> UbxMsgRateCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<UbxMsgRateCmdCapnp>();
    return UbxMsgRateCmd{static_cast<UBX_MSG::msg_id>(root.getId()), root.getPort(),
                         root.getRate()};
}

auto CapnpCodec<UbxMsgRateCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_MSG_RATE);
}

auto CapnpCodec<UbxGnssConfigCmd>::encode(const UbxGnssConfigCmd& event) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<UbxGnssConfigCmdCapnp>();
    auto cfgs = root.initConfigs(event.gnssConfigs.size());
    for (std::size_t i = 0; i < event.gnssConfigs.size(); ++i) {
        auto cfg = cfgs[i];
        cfg.setGnssId(event.gnssConfigs[i].gnssId);
        cfg.setResTrkCh(event.gnssConfigs[i].resTrkCh);
        cfg.setMaxTrkCh(event.gnssConfigs[i].maxTrkCh);
        cfg.setFlags(event.gnssConfigs[i].flags);
    }
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<UbxGnssConfigCmd>::decode(const std::vector<std::uint8_t>& data)
    -> UbxGnssConfigCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<UbxGnssConfigCmdCapnp>();
    UbxGnssConfigCmd event{};
    auto cfgs = root.getConfigs();
    event.gnssConfigs.reserve(cfgs.size());
    for (auto cfg : cfgs) {
        event.gnssConfigs.push_back(GnssConfigStruct{cfg.getGnssId(), cfg.getResTrkCh(),
                                                     cfg.getMaxTrkCh(), cfg.getFlags()});
    }
    return event;
}
auto CapnpCodec<UbxGnssConfigCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_GNSS_CONFIG);
}

auto CapnpCodec<UbxRateCmd>::encode(const UbxRateCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<UbxRateCmdCapnp>();
    root.setMeasRate(cmd.measRate);
    root.setNavRate(cmd.navRate);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<UbxRateCmd>::decode(const std::vector<std::uint8_t>& data) -> UbxRateCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<UbxRateCmdCapnp>();
    return UbxRateCmd{root.getMeasRate(), root.getNavRate()};
}

auto CapnpCodec<UbxRateCmd>::messageKey() -> std::uint16_t {
    return 0;
}

auto CapnpCodec<UbxResetCmd>::encode(const UbxResetCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<UbxResetCmdCapnp>();
    root.setResetFlags(cmd.resetFlags);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<UbxResetCmd>::decode(const std::vector<std::uint8_t>& data) -> UbxResetCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<UbxResetCmdCapnp>();
    UbxResetCmd cmd{};
    cmd.resetFlags = root.getResetFlags();
    return cmd;
}

auto CapnpCodec<UbxResetCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_RESET);
}

auto CapnpCodec<UbxSaveCmd>::encode(const UbxSaveCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<UbxSaveCmdCapnp>();
    root.setDevMask(cmd.devMask);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<UbxSaveCmd>::decode(const std::vector<std::uint8_t>& data) -> UbxSaveCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<UbxSaveCmdCapnp>();
    UbxSaveCmd cmd{};
    cmd.devMask = root.getDevMask();
    return cmd;
}

auto CapnpCodec<UbxSaveCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_UBX_CFG_SAVE);
}

auto CapnpCodec<UbxSetAopCmd>::encode(const UbxSetAopCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<UbxSetAopCmdCapnp>();
    root.setEnable(cmd.enable);
    root.setMaxOrbErr(cmd.maxOrbErr);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<UbxSetAopCmd>::decode(const std::vector<std::uint8_t>& data) -> UbxSetAopCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<UbxSetAopCmdCapnp>();
    return UbxSetAopCmd{root.getEnable(), root.getMaxOrbErr()};
}

auto CapnpCodec<UbxSetAopCmd>::messageKey() -> std::uint16_t {
    return 0;
}

auto CapnpCodec<MqttInhibitCmd>::encode(const MqttInhibitCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<MqttInhibitCmdCapnp>();
    root.setInhibit(cmd.inhibit);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}

auto CapnpCodec<MqttInhibitCmd>::decode(const std::vector<std::uint8_t>& data) -> MqttInhibitCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<MqttInhibitCmdCapnp>();
    return MqttInhibitCmd{root.getInhibit()};
}

auto CapnpCodec<MqttInhibitCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_MQTT_INHIBIT);
}

auto CapnpCodec<PolaritySwitchCmd>::encode(const PolaritySwitchCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<PolaritySwitchCmdCapnp>();
    root.setPol1(cmd.pol1);
    root.setPol2(cmd.pol2);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<PolaritySwitchCmd>::decode(const std::vector<std::uint8_t>& data)
    -> PolaritySwitchCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<PolaritySwitchCmdCapnp>();
    return PolaritySwitchCmd{root.getPol1(), root.getPol2()};
}
auto CapnpCodec<PolaritySwitchCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_POLARITY_SWITCH);
}

auto CapnpCodec<BiasVoltageCmd>::encode(const BiasVoltageCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<BiasVoltageCmdCapnp>();
    root.setVoltage(cmd.voltage);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<BiasVoltageCmd>::decode(const std::vector<std::uint8_t>& data) -> BiasVoltageCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<BiasVoltageCmdCapnp>();
    BiasVoltageCmd cmd{};
    cmd.voltage = root.getVoltage();
    return cmd;
}
auto CapnpCodec<BiasVoltageCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_BIAS_VOLTAGE);
}

auto CapnpCodec<BiasSwitchCmd>::encode(const BiasSwitchCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<BiasSwitchCmdCapnp>();
    root.setState(cmd.state);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<BiasSwitchCmd>::decode(const std::vector<std::uint8_t>& data) -> BiasSwitchCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<BiasSwitchCmdCapnp>();
    BiasSwitchCmd cmd{};
    cmd.state = root.getState();
    return cmd;
}
auto CapnpCodec<BiasSwitchCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_BIAS_SWITCH);
}

auto CapnpCodec<PreampSwitchCmd>::encode(const PreampSwitchCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<PreampSwitchCmdCapnp>();
    root.setChannel(cmd.channel);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<PreampSwitchCmd>::decode(const std::vector<std::uint8_t>& data) -> PreampSwitchCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<PreampSwitchCmdCapnp>();
    return PreampSwitchCmd{root.getChannel()};
}
auto CapnpCodec<PreampSwitchCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PREAMP_SWITCH);
}

auto CapnpCodec<AdcSampleTriggerCmd>::encode(const AdcSampleTriggerCmd& cmd)
    -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<AdcSampleTriggerCmdCapnp>();
    root.setChannel(cmd.channel);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<AdcSampleTriggerCmd>::decode(const std::vector<std::uint8_t>& data)
    -> AdcSampleTriggerCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<AdcSampleTriggerCmdCapnp>();
    return AdcSampleTriggerCmd{root.getChannel()};
}
auto CapnpCodec<AdcSampleTriggerCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_ADC_SAMPLE_REQUEST);
}

auto CapnpCodec<HistogramClearCmd>::encode(const HistogramClearCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<HistogramClearCmdCapnp>();
    root.setHistogramName(cmd.histogramName);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<HistogramClearCmd>::decode(const std::vector<std::uint8_t>& data)
    -> HistogramClearCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<HistogramClearCmdCapnp>();
    return HistogramClearCmd{root.getHistogramName().cStr()};
}
auto CapnpCodec<HistogramClearCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_HISTOGRAM_CLEAR);
}

auto CapnpCodec<GpioRateRequestCmd>::encode(const GpioRateRequestCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GpioRateRequestCmdCapnp>();
    root.setNPoints(cmd.n_points);
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<GpioRateRequestCmd>::decode(const std::vector<std::uint8_t>& data)
    -> GpioRateRequestCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<GpioRateRequestCmdCapnp>();
    return GpioRateRequestCmd{root.getNPoints()};
}
auto CapnpCodec<GpioRateRequestCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_GPIO_RATE_REQUEST);
}

auto CapnpCodec<EventTriggerCmd>::encode(const EventTriggerCmd& cmd) -> std::vector<uint8_t> {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<EventTriggerCmdCapnp>();
    root.setSignal(static_cast<std::uint8_t>(cmd.signal));
    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();
    return {bytes.begin(), bytes.end()};
}
auto CapnpCodec<EventTriggerCmd>::decode(const std::vector<std::uint8_t>& data) -> EventTriggerCmd {
    auto reader = makeReader(data);
    auto root = reader.getRoot<EventTriggerCmdCapnp>();
    return EventTriggerCmd{static_cast<GPIO_SIGNAL>(root.getSignal())};
}
auto CapnpCodec<EventTriggerCmd>::messageKey() -> std::uint16_t {
    return static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_EVENTTRIGGER);
}

#define EMPTY_CMD_CODEC(TYPE, CAPNP_TYPE, KEY)                                                     \
    auto CapnpCodec<TYPE>::encode(const TYPE&) -> std::vector<uint8_t> {                           \
        capnp::MallocMessageBuilder msg;                                                           \
        msg.initRoot<CAPNP_TYPE>();                                                                \
        auto flat = capnp::messageToFlatArray(msg);                                                \
        auto bytes = flat.asBytes();                                                               \
        return {bytes.begin(), bytes.end()};                                                       \
    }                                                                                              \
    auto CapnpCodec<TYPE>::decode(const std::vector<std::uint8_t>& data) -> TYPE {                 \
        auto reader = makeReader(data);                                                            \
        reader.getRoot<CAPNP_TYPE>();                                                              \
        return TYPE{};                                                                             \
    }                                                                                              \
    auto CapnpCodec<TYPE>::messageKey() -> std::uint16_t {                                         \
        return static_cast<std::uint16_t>(TCP_MSG_KEY::KEY);                                       \
    }

EMPTY_CMD_CODEC(PreampSwitchRequestCmd, PreampSwitchRequestCmdCapnp, MSG_PREAMP_SWITCH_REQUEST)
EMPTY_CMD_CODEC(GpioRateResetCmd, GpioRateResetCmdCapnp, MSG_GPIO_RATE_RESET)
EMPTY_CMD_CODEC(UbxConfigDefaultCmd, UbxConfigDefaultCmdCapnp, MSG_UBX_CONFIG_DEFAULT)
EMPTY_CMD_CODEC(I2CStatsRequestCmd, I2cStatsRequestCmdCapnp, MSG_I2C_STATS_REQUEST)
EMPTY_CMD_CODEC(I2CScanBusCmd, I2cScanBusCmdCapnp, MSG_I2C_SCAN_BUS)
EMPTY_CMD_CODEC(SPIStatsRequestCmd, SpiStatsRequestCmdCapnp, MSG_SPI_STATS_REQUEST)
EMPTY_CMD_CODEC(CalibRequestCmd, CalibRequestCmdCapnp, MSG_CALIB_REQUEST)
EMPTY_CMD_CODEC(CalibSaveCmd, CalibSaveCmdCapnp, MSG_CALIB_SAVE)
EMPTY_CMD_CODEC(GainSwitchRequestCmd, GainSwitchRequestCmdCapnp, MSG_GAIN_SWITCH_REQUEST)
EMPTY_CMD_CODEC(DacSettingRequestCmd, DacSettingRequestCmdCapnp, MSG_THRESHOLD_REQUEST)
EMPTY_CMD_CODEC(ThresholdSettingRequestCmd, ThresholdSettingRequestCmdCapnp, MSG_THRESHOLD_REQUEST)
EMPTY_CMD_CODEC(PcaSwitchRequestCmd, PcaSwitchRequestCmdCapnp, MSG_PCA_SWITCH_REQUEST)
EMPTY_CMD_CODEC(AdcModeRequestCmd, AdcModeRequestCmdCapnp, MSG_ADC_MODE_REQUEST)
EMPTY_CMD_CODEC(PolaritySwitchRequestCmd, PolaritySwitchRequestCmdCapnp,
                MSG_POLARITY_SWITCH_REQUEST)
EMPTY_CMD_CODEC(BiasVoltageRequestCmd, BiasVoltageRequestCmdCapnp, MSG_BIAS_VOLTAGE_REQUEST)
EMPTY_CMD_CODEC(BiasSwitchRequestCmd, BiasSwitchRequestCmdCapnp, MSG_BIAS_SWITCH_REQUEST)
EMPTY_CMD_CODEC(TemperatureRequestCmd, TemperatureRequestCmdCapnp, MSG_TEMPERATURE_REQUEST)
EMPTY_CMD_CODEC(DacEepromSetCmd, DacEepromSetCmdCapnp, MSG_DAC_EEPROM_SET)
EMPTY_CMD_CODEC(EventTriggerRequestCmd, EventTriggerRequestCmdCapnp, MSG_EVENTTRIGGER_REQUEST)
EMPTY_CMD_CODEC(HistogramRequestCmd, HistogramRequestCmdCapnp, MSG_HISTOGRAM_REQUEST)
EMPTY_CMD_CODEC(UbxMsgRateRequestCmd, UbxMsgRateRequestCmdCapnp, MSG_UBX_MSG_RATE_REQUEST)

#undef EMPTY_CMD_CODEC
