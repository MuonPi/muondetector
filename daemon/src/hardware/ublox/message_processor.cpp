#include "hardware/ublox/message_processor.h"
#include "core/logging/logger.h"
#include "data/ublox/ublox_messages.h"
#include "data/ublox/ublox_structs.h"
#include "utility/unixtime_from_gps.h"
#include "data/custom_io_operators.h"
// #include <custom_io_operators.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <type_traits>
#include <string>
#include <string_view>
#include <optional>
#include <charconv>
#include <cctype>

template <class T, class = void> struct is_iterator : std::false_type
{
};

template <class T>
struct is_iterator<T, std::void_t<typename std::iterator_traits<T>::iterator_category>> : std::true_type
{
};

enum class endian : bool
{
    big,
    little
};

template <typename T, endian Endian = endian::little, typename It,
          std::enable_if_t<std::is_integral<T>::value, bool> = true,
          std::enable_if_t<is_iterator<It>::value, bool> = true>
[[nodiscard]] auto get(const It &start) -> T
{
    const auto &end{start + sizeof(T)};
    T value{0};
    std::size_t shift{(Endian == endian::little) ? 0 : (sizeof(T) - 1) * 8};
    for (auto it = start; it != end; it++)
    {
        value += static_cast<T>(*it) << shift;
        if (Endian == endian::little)
        {
            shift += 8;
        }
        else
        {
            shift -= 8;
        }
    }
    return value;
}

template <typename T, endian Endian = endian::little, typename It,
          std::enable_if_t<std::is_integral<T>::value, bool> = true,
          std::enable_if_t<is_iterator<It>::value, bool> = true>
void put(const It &start, const T &value)
{
    const auto &end{start + sizeof(T)};
    std::size_t shift{(Endian == endian::little) ? 0 : (sizeof(T) - 1) * 8};
    for (auto it = start; it != end; it++)
    {
        *it = static_cast<std::uint8_t>((value >> shift) & 0xff);
        if (Endian == endian::little)
        {
            shift += 8;
        }
        else
        {
            shift -= 8;
        }
    }
}

std::optional<UbxMessage> MessageProcessor::msgWaitingForAck{std::nullopt};
std::optional<GpsVersion> MessageProcessor::gpsVersion{std::nullopt};

auto MessageProcessor::processMessage(const UbxMessage &msg) -> std::optional<UbxEvent>
{
    static const std::map<std::uint8_t, const char *> ubx_class_names{
        {0x01, "UBX-NAV"}, {0x02, "UBX-RXM"}, {0x04, "UBX-INF"}, {0x05, "UBX-ACK"}, {0x06, "UBX-CFG"},
        {0x09, "UBX-UPD"}, {0x10, "UBX-ESF"}, {0x13, "UBX-MGA"}, {0x0a, "UBX-MON"}, {0x0b, "UBX-AID"},
        {0x0d, "UBX-TIM"}, {0x21, "UBX-LOG"}, {0x27, "UBX-SEC"}, {0x28, "UBX-HNR"}};

    const std::map<uint16_t, std::pair<std::function<std::optional<UbxEvent>()>, std::string>> handlerLookup {
        {{UBX_MSG::NAV_STATUS, std::make_pair([&] { return UBXNavStatus(msg.payload()); }, "UBX-NAV-STATUS")},
         {UBX_MSG::NAV_DOP, std::make_pair([&] { return UBXNavDOP(msg.payload()); }, "UBX-NAV-DOP")},
         {UBX_MSG::NAV_TIMEGPS, std::make_pair([&] { return UBXNavTimeGPS(msg.payload()); }, "UBX-NAV-TIMEGPS")},
         {UBX_MSG::NAV_TIMEUTC, std::make_pair([&] { return UBXNavTimeUTC(msg.payload()); }, "UBX-NAV-TIMEUTC")},
         {UBX_MSG::NAV_CLOCK, std::make_pair([&] { return UBXNavClock(msg.payload()); }, "UBX-NAV-CLOCK")},
         {UBX_MSG::NAV_SVINFO, std::make_pair([&] { return UBXNavSVinfo(msg.payload()); }, "UBX-NAV-SVINFO")},
         {UBX_MSG::NAV_SAT, std::make_pair([&] { return UBXNavSat(msg.payload()); }, "UBX-NAV-SAT")},
         {UBX_MSG::NAV_POSLLH, std::make_pair([&] { return UBXNavPosLLH(msg.payload()); }, "UBX-NAV-POSLLH")}

         ,
         {UBX_MSG::CFG_ANT, std::make_pair([&] { return UBXCfgAnt(msg.payload()); }, "UBX-CFG-ANT")},
         {UBX_MSG::CFG_NAVX5, std::make_pair([&] { return UBXCfgNavX5(msg.payload()); }, "UBX-CFG-NAVX5")},
         {UBX_MSG::CFG_NAV5, std::make_pair([&] { return UBXCfgNav5(msg.payload()); }, "UBX-CFG-NAV5")},
         {UBX_MSG::CFG_TP5, std::make_pair([&] { return UBXCfgTP5(msg.payload()); }, "UBX-CFG-TP5")},
         {UBX_MSG::CFG_GNSS, std::make_pair([&] { return UBXCfgGNSS(msg.payload()); }, "UBX-CFG-GNSS")},
         {UBX_MSG::CFG_MSG, std::make_pair([&] { return UBXCfgMSG(msg.payload()); }, "UBX-CFG-MSG")}

         ,
         {UBX_MSG::MON_RXBUF, std::make_pair([&] { return UBXMonRx(msg.payload()); }, "UBX-MON-RXBUF")},
         {UBX_MSG::MON_TXBUF, std::make_pair([&] { return UBXMonTx(msg.payload()); }, "UBX-MON-TXBUF")},
         {UBX_MSG::MON_HW, std::make_pair([&] { return UBXMonHW(msg.payload()); }, "UBX-MON-HW")},
         {UBX_MSG::MON_HW2, std::make_pair([&] { return UBXMonHW2(msg.payload()); }, "UBX-MON-HW2")},
         {UBX_MSG::MON_VER, std::make_pair([&] { return UBXMonVer(msg.payload()); }, "UBX-MON-VER")}

         ,
         {UBX_MSG::TIM_TP, std::make_pair([&] { return UBXTimTP(msg.payload()); }, "UBX-TIM-TP")},
         {UBX_MSG::TIM_TM2, std::make_pair([&] { return UBXTimTM2(msg.payload()); }, "UBX-TIM-TM2")}}};

    if (handlerLookup.count(msg.full_id()) > 0)
    {
        const auto &[handle, name] = handlerLookup.at(msg.full_id());
        if (logLevel() == LogLevel::Debug)
        {
            std::stringstream sstr{};
            sstr << "received " << name << " message (0x" << std::hex << std::setfill('0') << std::setw(2)
                 << static_cast<unsigned>(msg.class_id()) << " 0x" << std::hex
                 << static_cast<unsigned>(msg.message_id()) << ")";
            logInfo(sstr.str());
        }
        return handle();
    }
    else if (msg.class_id() == 0x05)
    {
        // Handle acknowledge messages
        if (msg.payload().size() < 2)
        {
            logWarn("received UBX-ACK message but data is corrupted");
            return std::nullopt;
        }
        if (msgWaitingForAck.has_value() == false)
        {
            // Got acknowledge message from ublox without requesting anything
            std::stringstream sstr{};
            sstr << "received ACK message but no message is waiting for Ack (msgID: 0x";
            sstr << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[0] << " 0x" << std::setfill('0')
                 << std::setw(2) << std::hex << (int)msg.payload()[1] << ")\n";
            logWarn(sstr.str());
            return std::nullopt;
        }
        if (logLevel() == LogLevel::Debug)
        {
            std::stringstream sstr{};
            sstr << "received UBX-ACK-ACK message about msgID: 0x";
            sstr << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[0] << " 0x" << std::setfill('0')
                 << std::setw(2) << std::hex << (int)msg.payload()[1];
            logInfo(sstr.str());
        }

        auto ackedMsgID = (uint16_t)(msg.payload()[0]) << 8U | msg.payload()[1];
        if (ackedMsgID != msgWaitingForAck.value().full_id())
        {
            std::stringstream sstr{};
            sstr << "received unexpected UBX-ACK message about msgID: 0x";
            sstr << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[0] << " 0x" << std::setfill('0')
                 << std::setw(2) << std::hex << (int)msg.payload()[1] << "\n";
            logWarn(sstr.str());
            return std::nullopt;
        }
        if (msg.message_id() == 0x00)
        {
            // emit UBXReceivedAckNak(msgWaitingForAck->full_id(),
            //                        (uint16_t)(msgWaitingForAck->payload()[0]) << 8U |
            //                        msgWaitingForAck->payload()[1]);
        }
        // ackTimer->stop(); // TODO : implement ack timer logic
        msgWaitingForAck = std::nullopt;
        if (logLevel() == LogLevel::Debug)
        {
            logInfo("processMessage: deleted message after ACK/NACK");
        }
    }
    else
    {
        unhandled(msg, ubx_class_names);
    }
    return std::nullopt;
}

void MessageProcessor::unhandled(const UbxMessage &msg, const std::map<std::uint8_t, const char *> ubx_class_names)
{

    if (msg.class_id() == 0x06)
    {
        if (logLevel() == LogLevel::Debug)
        {
            std::stringstream sstr{};
            sstr << "received unhandled UBX-CFG message:";
            for (std::string::size_type i = 0; i < msg.payload().size(); i++)
            {
                sstr << " 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[i];
            }
            logDebug(sstr.str());
        }
    }
    else
    {
        if (logLevel() == LogLevel::Debug)
        {
            std::stringstream sstr{};
            if (ubx_class_names.count(msg.class_id()) > 0)
            {
                sstr << "received unhandled " << ubx_class_names.at(msg.class_id()) << " message"
                     << " (0x" << std::hex << std::setfill('0') << std::setw(2) << (int)msg.class_id() << " 0x"
                     << std::hex << (int)msg.message_id() << ")";
            }
            else
            {
                sstr << "received unknown UBX message (0x" << std::hex << std::setfill('0') << std::setw(2)
                     << (int)msg.class_id() << " 0x" << std::hex << (int)msg.message_id() << ")";
            }
            logDebug(sstr.str());
        }
    }
}

auto MessageProcessor::UBXNavStatus(const std::string &msg) -> std::optional<UbxEvent>
{
    // UBX-NAV_STATUS: RX status information
    // parse all fields
    NavStatus data;
    data.iTOW = get<std::uint32_t>(msg.begin());
    data.gpsFix = get<std::uint8_t>(msg.begin() + 4);
    data.flags = get<std::uint8_t>(msg.begin() + 5);
    data.flags2 = get<std::uint8_t>(msg.begin() + 7);
    data.ttff = get<std::uint32_t>(msg.begin() + 8);
    data.msss = get<std::uint32_t>(msg.begin() + 12);

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX NAV-STATUS message:" << '\n';
        sstr << " iTOW             : " << data.iTOW / 1000 << " s" << '\n';
        sstr << " gpsFix           : " << (int)data.gpsFix << '\n';
        sstr << " time to first fix: " << (float)data.ttff / 1000. << " s" << '\n';
        sstr << " uptime           : " << (float)data.msss / 1000. << " s" << '\n';
        sstr << " flags            : " << std::hex << "0x" << (int)data.flags << std::dec << '\n';
        sstr << " flags2           : " << std::hex << "0x" << (int)data.flags2 << std::dec << '\n';
        logDebug(sstr.str());
    }

    return data;
}

auto MessageProcessor::UBXNavDOP(const std::string &msg) -> std::optional<UbxEvent>
{
    // UBX-NAV-DOP: dilution of precision values
    UbxDopStruct data{};

    // parse all fields
    auto iTOW{get<std::uint32_t>(msg.begin())};
    // geometric DOP
    data.gDOP = get<std::uint16_t>(msg.begin() + 4);
    // position DOP
    data.pDOP = get<std::uint16_t>(msg.begin() + 6);
    // time DOP
    data.tDOP = get<std::uint16_t>(msg.begin() + 8);
    // vertical DOP
    data.vDOP = get<std::uint16_t>(msg.begin() + 10);
    // horizontal DOP
    data.hDOP = get<std::uint16_t>(msg.begin() + 12);
    // northing DOP
    data.nDOP = get<std::uint16_t>(msg.begin() + 14);
    // easting DOP
    data.eDOP = get<std::uint16_t>(msg.begin() + 16);

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX NAV-DOP message:" << '\n';
        sstr << " iTOW             : " << std::dec << iTOW / 1000. << " s" << '\n';
        sstr << " geometric DOP    : " << std::dec << data.gDOP / 100. << '\n';
        sstr << " position DOP     : " << std::dec << data.pDOP / 100. << '\n';
        sstr << " time DOP         : " << std::dec << data.tDOP / 100. << '\n';
        sstr << " vertical DOP     : " << std::dec << data.vDOP / 100. << '\n';
        sstr << " horizontal DOP   : " << std::dec << data.hDOP / 100. << '\n';
        sstr << " northing DOP     : " << std::dec << data.nDOP / 100. << '\n';
        sstr << " easting DOP      : " << std::dec << data.eDOP / 100. << '\n';
        logDebug(sstr.str());
    }

    return data;
}

auto MessageProcessor::UBXNavTimeGPS(const std::string &msg) -> std::optional<UbxEvent>
{
    // parse all fields
    // GPS time of week
    NavTimeGPS data;
    data.iTOW = get<std::uint32_t>(msg.begin());

    data.fTOW = get<std::int32_t>(msg.begin() + 4);

    data.wnR = get<std::uint16_t>(msg.begin() + 8);
    data.leapS = get<std::int8_t>(msg.begin() + 10);
    data.flags = get<std::uint8_t>(msg.begin() + 11);

    // time accuracy estimate
    data.tAcc = get<std::uint32_t>(msg.begin() + 12);

    double sr = data.iTOW / 1000.;
    sr = sr - data.iTOW / 1000;

    // meaning of columns:
    // 01 20 - signature of NAV-TIMEGPS message
    // week nr, second in current week, ns of timestamp in current second,
    // nr of leap seconds wrt UTC, accuracy (ns)

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX-NAV-TIMEGPS message:" << '\n';
        sstr << " week nr       : " << std::dec << data.wnR << '\n';
        sstr << " iTOW          : " << std::dec << data.iTOW << " ms = " << data.iTOW / 1000 << " s" << '\n';
        sstr << " fTOW          : " << std::dec << data.fTOW << " = " << (long int)(sr * 1e9 + data.fTOW) << " ns"
             << '\n';
        sstr << " leap seconds  : " << std::dec << (int)data.leapS << " s" << '\n';
        sstr << " time accuracy : " << std::dec << data.tAcc << " ns" << '\n';
        sstr << " flags             : ";
        for (int i = 7; i >= 0; i--)
            if (data.flags & 1 << i)
                sstr << i;
            else
                sstr << "-";
        sstr << '\n';
        sstr << "   tow valid        : " << std::string((data.flags & 1) ? "yes" : "no") << '\n';
        sstr << "   week valid       : " << std::string((data.flags & 2) ? "yes" : "no") << '\n';
        sstr << "   leap sec valid   : " << std::string((data.flags & 4) ? "yes" : "no");
        logDebug(sstr.str());
        sstr.clear();

        // emit gpsPropertyUpdatedUint32(tAcc, timeAccuracy.updateAge(), 'a');
        // timeAccuracy = tAcc;
        // timeAccuracy.lastUpdate = std::chrono::system_clock::now();
        // if (flags & 4) {
        //     leapSeconds = leapS;
        // }

        struct timespec ts = unixtime_from_gps(data.wnR, data.iTOW / 1000, (long int)(sr * 1e9 + data.fTOW));
        sstr << ts.tv_sec << '.' << ts.tv_nsec << "\n";
        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXNavTimeUTC(const std::string &msg) -> std::optional<UbxEvent>
{
    // parse all fields
    // GPS time of week
    NavTimeUTC data;
    data.iTOW = get<std::uint32_t>(msg.begin());

    // time accuracy estimate
    data.tAcc = get<std::uint32_t>(msg.begin() + 4);
    data.nano = get<std::int32_t>(msg.begin() + 8);
    data.year = get<std::uint16_t>(msg.begin() + 12);

    data.month = static_cast<uint16_t>(msg[14]);
    data.day = static_cast<uint16_t>(msg[15]);
    data.hour = static_cast<uint16_t>(msg[16]);
    data.min = static_cast<uint16_t>(msg[17]);
    data.sec = static_cast<uint16_t>(msg[18]);

    std::uint8_t flags = static_cast<uint16_t>(msg[19]);

    double sr = data.iTOW / 1000.;
    sr = sr - data.iTOW / 1000;

    // meaning of columns:
    // 01 21 - signature of NAV-TIMEUTC message
    // second in current week, year, month, day, hour, minute, seconds(+fraction)
    // accuracy (ns)

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX-NAV-TIMEUTC message:" << '\n';
        sstr << " iTOW           : " << data.iTOW << " ms = " << data.iTOW / 1000 << " s" << '\n';
        sstr << " nano           : " << data.nano << " ns" << '\n';
        sstr << " date y/m/d     : " << data.year << "/" << data.month << "/" << data.day << '\n';
        sstr << " UTC time h:m:s : " << std::setw(2) << std::setfill('0') << data.hour << ":" << data.min << ":";
        sstr << static_cast<int>(data.sec + data.nano * 1e-9) << '\n';
        sstr << " time accuracy : " << data.tAcc << " ns" << '\n';
        sstr << " flags";
        for (int i = 7; i >= 0; i--)
        {
            if (flags & 1 << i)
            {
                sstr << i;
            }
            else
            {
                sstr << "-";
            }
        }
        sstr << '\n';
        sstr << "   tow valid        : " << std::string((flags & 1) ? "yes" : "no") << '\n';
        sstr << "   week valid       : " << std::string((flags & 2) ? "yes" : "no") << '\n';
        sstr << "   UTC time valid   : " << std::string((flags & 4) ? "yes" : "no") << '\n';
        std::string utcStd;
        switch ((flags & 0xf0) >> 4)
        {
        case 0:
            utcStd = "n/a";
            break;
        case 1:
            utcStd = "CRL";
            break;
        case 2:
            utcStd = "NIST";
            break;
        case 3:
            utcStd = "USNO";
            break;
        case 4:
            utcStd = "BIPM";
            break;
        case 5:
            utcStd = "EU";
            break;
        case 6:
            utcStd = "SU";
            break;
        case 7:
            utcStd = "NTSC";
            break;
        default:
            utcStd = "unknown";
        }
        sstr << "   UTC standard  : " << utcStd << "\n";
        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXNavClock(const std::string &msg) -> std::optional<UbxEvent>
{
    // parse all fields
    // GPS time of week
    NavClock data;
    data.iTOW = get<std::uint32_t>(msg.begin());
    // clock bias
    data.clkB = get<std::int32_t>(msg.begin() + 4);
    // clock drift
    data.clkD = get<std::int32_t>(msg.begin() + 8);
    // time accuracy estimate
    data.tAcc = get<std::uint32_t>(msg.begin() + 12);
    // freq accuracy estimate
    data.fAcc = get<std::uint32_t>(msg.begin() + 16);

    // meaning of columns:
    // 01 22 - signature of NAV-CLOCK message
    // second in current week (s), clock bias (ns), clock drift (ns/s), time accuracy (ns), freq accuracy (ps/s)

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "clkB[0]=" << std::setfill('0') << std::setw(2) << std::hex << (int)msg[4] << '\n';
        sstr << "clkB[1]=" << std::setfill('0') << std::setw(2) << std::hex << (int)msg[5] << '\n';
        sstr << "clkB[2]=" << std::setfill('0') << std::setw(2) << std::hex << (int)msg[6] << '\n';
        sstr << "clkB[3]=" << std::setfill('0') << std::setw(2) << std::hex << (int)msg[7];
        sstr << "*** UBX-NAV-CLOCK message:" << '\n';
        sstr << " iTOW          : " << data.iTOW / 1000 << " s" << '\n';
        sstr << " clock bias    : " << data.clkB << " ns" << '\n';
        sstr << " clock drift   : " << data.clkD << " ns/s" << '\n';
        sstr << " time accuracy : " << data.tAcc << " ns" << '\n';
        sstr << " freq accuracy : " << data.fAcc << " ps/s\n";
        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXNavSVinfo(const std::string &msg) -> std::optional<UbxEvent>
{
    bool allSats = true;
    NavSat data;
    // UBX-NAV-SVINFO: satellite information
    // parse all fields
    // GPS time of week
    data.iTOW = get<uint32_t>(msg.begin());
    data.numSvs = get<std::uint8_t>(msg.begin() + 4);
    data.globFlags = get<std::uint8_t>(msg.begin() + 5);

    const std::size_t N{(msg.size() - 8) / 12};

    for (std::size_t i = 0; i < N; i++)
    {
        const std::size_t n{8 + i * 12};

        auto satId{get<std::uint8_t>(msg.begin() + n + 1)};
        auto flags{get<std::uint8_t>(msg.begin() + n + 2)};
        auto quality{get<std::uint8_t>(msg.begin() + n + 3)};
        auto cnr{get<std::uint8_t>(msg.begin() + n + 4)};
        auto elev{get<std::int8_t>(msg.begin() + n + 5)};
        auto azim{get<std::int16_t>(msg.begin() + n + 6)};
        auto prRes{static_cast<float>(get<int32_t>(msg.begin() + n + 8)) / 100.0F};

        bool used = false;
        if (flags & 0x01)
            used = true;
        std::uint8_t health = (flags >> 4 & 0x01);
        health += 1;
        std::uint8_t orbitSource = 0;
        if (flags & 0x04)
        {
            if (flags & 0x08)
                orbitSource = 1;
            else if (flags & 0x20)
                orbitSource = 2;
            else if (flags & 0x40)
                orbitSource = 3;
        }
        bool smoothed = (flags & 0x80);
        bool diffCorr = (flags & 0x02);

        int gnssId{[&satId] {
            if (satId < 33)
            {
                return 0;
            }
            else if (satId < 65)
            {
                return 3;
            }
            else if (satId < 97 || satId == 255)
            {
                return 6;
            }
            else if (satId < 159)
            {
                return 1;
            }
            else if (satId < 164)
            {
                return 3;
            }
            else if (satId < 183)
            {
                return 4;
            }
            else if (satId < 198)
            {
                return 5;
            }
            else if (satId < 247)
            {
                return 2;
            }
            return 7;
        }()};

        GnssSatellite sat(gnssId, satId, cnr, elev, azim, prRes, quality, health, orbitSource, used, diffCorr,
                          smoothed);
        if (sat.Cnr > 0)
        {
            data.goodSats++;
        }
        data.satellites.push_back(std::move(sat));
    }
    if (!allSats)
    {
        std::sort(data.satellites.begin(), data.satellites.end(), GnssSatellite::sortByCnr);
        while (!data.satellites.empty() && (data.satellites.back().Cnr == 0))
        {
            data.satellites.pop_back();
        }
    }

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << std::setfill(' ') << std::setw(3);
        sstr << "*** UBX-NAV-SVINFO message:" << '\n';
        sstr << " iTOW          : " << data.iTOW / 1000 << " s" << '\n';
        sstr << " global flags  : 0x" << std::hex << (int)data.globFlags.value() << std::dec << '\n';
        sstr << " Nr of sats    : " << (int)data.numSvs << "  (nr of sections=" << N << ")\n";
        logDebug(sstr.str());
        sstr.clear();

        GnssSatellite::PrintHeader(true);
        for (std::vector<GnssSatellite>::iterator it = data.satellites.begin(); it != data.satellites.end(); it++)
        {
            it->Print(distance(data.satellites.begin(), it), false);
        }
        sstr << "   Sat Data :\n";
        sstr << "   --------------------------------------------------------------------\n";
        sstr << " Nr of avail sats : " << data.goodSats << "\n";
        logDebug(sstr.str());
    }

    return data;
}

auto MessageProcessor::UBXNavSat(const std::string &msg) -> std::optional<UbxEvent>
{
    bool allSats = true;

    NavSat data;
    // UBX-NAV-SAT: satellite information
    // parse all fields
    // GPS time of week
    data.iTOW = get<std::uint32_t>(msg.begin());
    // version
    auto version{get<std::uint8_t>(msg.begin() + 4)};
    auto numSvs{get<std::uint8_t>(msg.begin() + 5)};

    const std::size_t N{(msg.size() - 8) / 12};

    for (std::size_t i = 0; i < N; i++)
    {
        const std::size_t n{8 + i * 12};

        auto gnssId{get<std::uint8_t>(msg.begin() + n)};
        auto satId{get<std::uint8_t>(msg.begin() + n + 1)};
        auto cnr{get<std::uint8_t>(msg.begin() + n + 2)};
        auto elev{get<std::int8_t>(msg.begin() + n + 3)};
        auto azim{get<std::int16_t>(msg.begin() + n + 4)};
        auto prRes{static_cast<float>(get<std::int16_t>(msg.begin() + n + 6)) / 10.0F};
        auto flags{get<std::uint32_t>(msg.begin() + n + 8)};

        GnssSatellite sat(gnssId, satId, cnr, elev, azim, prRes, flags);

        if (sat.Cnr > 0)
        {
            data.goodSats++;
        }
        data.satellites.push_back(std::move(sat));
    }
    if (!allSats)
    {
        std::sort(data.satellites.begin(), data.satellites.end(), GnssSatellite::sortByCnr);
        while (!data.satellites.empty() && (data.satellites.back().Cnr == 0))
        {
            data.satellites.pop_back();
        }
    }

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "   --------------------------------------------------------------------\n";
        sstr << " Nr of avail sats : " << (int)data.goodSats << "\n";
        logDebug(sstr.str());
        sstr.clear();

        GnssSatellite::PrintHeader(true);
        for (std::vector<GnssSatellite>::iterator it = data.satellites.begin(); it != data.satellites.end(); it++)
        {
            it->Print(distance(data.satellites.begin(), it), false);
        }
        sstr << std::setfill(' ') << std::setw(3);
        sstr << "*** UBX-NAV-SAT message:" << '\n';
        sstr << " iTOW          : " << data.iTOW / 1000 << " s" << '\n';
        sstr << " version       : " << (int)version << '\n';
        sstr << " Nr of sats    : " << (int)numSvs << "  (nr of sections=" << N << ")\n";
        sstr << "   Sat Data :\n";
        logDebug(sstr.str());
    }

    return data;
}

auto MessageProcessor::UBXNavPosLLH(const std::string &msg) -> std::optional<UbxEvent>
{
    GnssPosStruct pos{};
    // GPS time of week
    pos.iTOW = get<decltype(pos.iTOW)>(msg.begin());
    // longitude in 1e-7 precision
    pos.lon = get<decltype(pos.lon)>(msg.begin() + 4);
    // latitude in 1e-7 precision
    pos.lat = get<decltype(pos.lat)>(msg.begin() + 8);
    // height above ellipsoid
    pos.height = get<decltype(pos.height)>(msg.begin() + 12);
    // height above main sea-level
    pos.hMSL = get<decltype(pos.hMSL)>(msg.begin() + 16);
    // horizontal accuracy estimate
    pos.hAcc = get<decltype(pos.hAcc)>(msg.begin() + 20);
    // vertical accuracy estimate
    pos.vAcc = get<decltype(pos.vAcc)>(msg.begin() + 24);
    return pos;
}

auto MessageProcessor::UBXCfgAnt(const std::string &msg) -> std::optional<UbxEvent>
{
    CfgAnt data;
    // parse all fields
    data.flags = get<std::uint16_t>(msg.begin());
    data.pins = get<std::uint16_t>(msg.begin() + 2);

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX-CFG-ANT message:" << '\n';
        sstr << " flags                     : 0x" << std::hex << (int)data.flags << std::dec << '\n';
        sstr << " ant supply control signal : " << std::string((data.flags & 0x01) ? "on" : "off") << '\n';
        sstr << " short detection           : " << std::string((data.flags & 0x02) ? "on" : "off") << '\n';
        sstr << " open detection            : " << std::string((data.flags & 0x04) ? "on" : "off") << '\n';
        sstr << " pwr down on short         : " << std::string((data.flags & 0x08) ? "on" : "off") << '\n';
        sstr << " auto recovery from short  : " << std::string((data.flags & 0x10) ? "on" : "off") << '\n';
        sstr << " supply switch pin         : " << (int)(data.pins & 0x1f) << '\n';
        sstr << " short detection pin       : " << (int)((data.pins >> 5) & 0x1f) << '\n';
        sstr << " open detection pin        : " << (int)((data.pins >> 10) & 0x1f) << '\n';

        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXCfgNavX5(const std::string &msg) -> std::optional<UbxEvent>
{
    CfgNavX5 data;
    // parse all fields
    data.version = get<std::uint8_t>(msg.begin());
    // auto mask1 { get<uint16_t>(msg.begin() + 1) };
    data.minSVs = get<std::uint8_t>(msg.begin() + 10);
    data.maxSVs = get<std::uint8_t>(msg.begin() + 11);
    data.minCNO = get<std::uint8_t>(msg.begin() + 12);
    data.iniFix3D = get<std::uint8_t>(msg.begin() + 14);
    data.wknRollover = get<std::uint16_t>(msg.begin() + 18);
    data.aopCfg = get<std::uint8_t>(msg.begin() + 27);
    data.aopOrbMaxErr = get<std::uint16_t>(msg.begin() + 30);

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX-MON-NAVX5 message:" << '\n';
        sstr << " msg version         : " << (int)data.version << '\n';
        sstr << " min nr of SVs       : " << (int)data.minSVs << '\n';
        sstr << " max nr of SVs       : " << (int)data.maxSVs << '\n';
        sstr << " min CNR for nav     : " << (int)data.minCNO << '\n';
        sstr << " initial 3D fix      : " << (int)data.iniFix3D << '\n';
        sstr << " GPS week rollover   : " << (int)data.wknRollover << '\n';
        sstr << " AOP auton config    : " << (int)data.aopCfg << '\n';
        sstr << " max AOP orbit error : " << (int)data.aopOrbMaxErr << " m" << '\n';
        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXCfgNav5(const std::string &msg) -> std::optional<UbxEvent>
{
    CfgNav5 data;
    // UBX CFG-NAV5: satellite information
    // parse all fields
    data.mask = get<uint16_t>(msg.begin());
    data.dynModel = get<std::uint8_t>(msg.begin() + 2);
    data.fixMode = get<std::uint8_t>(msg.begin() + 3);
    data.fixedAlt = get<int32_t>(msg.begin() + 4);
    data.fixedAltVar = get<uint32_t>(msg.begin() + 8);
    data.minElev = get<int8_t>(msg.begin() + 12);
    data.cnoThreshNumSVs = get<std::uint8_t>(msg.begin() + 24);
    data.cnoThresh = get<std::uint8_t>(msg.begin() + 25);

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX CFG-NAV5 message:" << '\n';
        sstr << " mask               : " << std::hex << (int)data.mask << std::dec << '\n';
        sstr << " dynamic model used : " << (int)data.dynModel << '\n';
        sstr << " fixMode            : " << (int)data.fixMode << '\n';
        sstr << " fixed Alt          : " << (double)data.fixedAlt * 0.01 << " m" << '\n';
        sstr << " fixed Alt Var      : " << (double)data.fixedAltVar * 0.0001 << " m^2" << '\n';
        sstr << " min elevation      : " << (int)data.minElev << " deg\n";
        sstr << " cnoThresh required for fix : " << (int)data.cnoThresh << " dBHz\n";
        sstr << " min nr of SVs having cnoThresh for fix : " << (int)data.cnoThreshNumSVs << '\n';
        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXCfgTP5(const std::string &msg) -> std::optional<UbxEvent>
{
    UbxTimePulseStruct tp{};
    // parse all fields
    tp.tpIndex = get<std::uint8_t>(msg.begin());
    tp.version = get<std::uint8_t>(msg.begin() + 1);
    tp.antCableDelay = get<int16_t>(msg.begin() + 4);
    tp.rfGroupDelay = get<int16_t>(msg.begin() + 6);
    tp.freqPeriod = get<uint32_t>(msg.begin() + 8);
    tp.freqPeriodLock = get<uint32_t>(msg.begin() + 12);
    tp.pulseLenRatio = get<uint32_t>(msg.begin() + 16);
    tp.pulseLenRatioLock = get<uint32_t>(msg.begin() + 20);
    tp.userConfigDelay = get<int32_t>(msg.begin() + 24);
    tp.flags = get<uint32_t>(msg.begin() + 28);

    bool isFreq{(tp.flags & 0x08u) != 0x00u};
    bool isLength{(tp.flags & 0x10) != 0x00u};

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX-CFG-TP5 message:" << '\n';
        sstr << " message version           : " << std::dec << (int)tp.version << '\n';
        sstr << " time pulse index          : " << std::dec << (int)tp.tpIndex << '\n';
        sstr << " ant cable delay           : " << std::dec << (int)tp.antCableDelay << " ns" << '\n';
        sstr << " rf group delay            : " << std::dec << (int)tp.rfGroupDelay << " ns" << '\n';
        sstr << " user config delay         : " << std::dec << (int)tp.userConfigDelay << " ns" << '\n';
        if (isFreq)
        {
            sstr << " pulse frequency           : " << std::dec << (int)tp.freqPeriod << " Hz" << '\n';
            sstr << " locked pulse frequency    : " << std::dec << (int)tp.freqPeriodLock << " Hz" << '\n';
        }
        else
        {
            sstr << " pulse period              : " << std::dec << (int)tp.freqPeriod << " us" << '\n';
            sstr << " locked pulse period       : " << std::dec << (int)tp.freqPeriodLock << " us" << '\n';
        }
        if (isLength)
        {
            sstr << " pulse length              : " << std::dec << (int)tp.pulseLenRatio << " us" << '\n';
            sstr << " locked pulse length       : " << std::dec << (int)tp.pulseLenRatioLock << " us" << '\n';
        }
        else
        {
            sstr << " pulse duty cycle          : " << std::dec << (double)tp.pulseLenRatio / ((uint64_t)1 << 32)
                 << '\n';
            sstr << " locked pulse duty cycle   : " << std::dec << (double)tp.pulseLenRatioLock / ((uint64_t)1 << 32)
                 << '\n';
        }
        sstr << " flags                     : 0x" << std::hex << (int)tp.flags << std::dec << '\n';
        sstr << " tp active                 : " << std::string((tp.flags & 0x01) ? "yes" : "no") << '\n';

        sstr << " lockGpsFreq               : " << std::string((tp.flags & 0x02) ? "on" : "off") << '\n';
        sstr << " lockedOtherSet            : " << std::string((tp.flags & 0x04) ? "on" : "off") << '\n';
        sstr << " isFreq                    : " << std::string((tp.flags & 0x08) ? "on" : "off") << '\n';
        sstr << " isLength                  : " << std::string((tp.flags & 0x10) ? "on" : "off") << '\n';
        sstr << " alignToTow                : " << std::string((tp.flags & 0x20) ? "on" : "off") << '\n';
        sstr << " polarity                  : " << std::string((tp.flags & 0x40) ? "rising" : "falling") << '\n';
        sstr << " time grid                 : ";
        if (gpsVersion.has_value() && getProtVersion(gpsVersion.value().prot).has_value() && getProtVersion(gpsVersion.value().prot).value().major < 16)
            sstr << std::string((tp.flags & 0x80) ? "GPS" : "UTC") << '\n';
        else
        {
            int timeGrid = (tp.flags & UbxTimePulseStruct::GRID_UTC_GPS) >> 7;
            switch (timeGrid)
            {
            case 0:
                sstr << "UTC" << '\n';
                break;
            case 1:
                sstr << "GPS" << '\n';
                break;
            case 2:
                sstr << "Glonass" << '\n';
                break;
            case 3:
                sstr << "BeiDou" << '\n';
                break;
            case 4:
                sstr << "Galileo" << '\n';
                break;
            default:
                sstr << "unknown" << '\n';
            }
        }

        logDebug(sstr.str());
    }
    return tp;
}

auto MessageProcessor::UBXCfgGNSS(const std::string &msg) -> std::optional<UbxEvent>
{
    // UBX-CFG-GNSS: GNSS configuration
    // parse all fields
    // version
    // send:
    // "0,0,ff,1,6,5,ff,0,1,0,0,0"
    CfgGNSS data;
    data.version = get<std::uint8_t>(msg.begin());
    data.numTrkChHw = get<std::uint8_t>(msg.begin() + 1);
    data.numTrkChUse = get<std::uint8_t>(msg.begin() + 2);
    data.numConfigBlocks = get<std::uint8_t>(msg.begin() + 3);

    const std::size_t N{(msg.size() - 4) / 8};

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX CFG-GNSS message:" << '\n';
        sstr << " version                    : " << (int)data.version << '\n';
        sstr << " nr of hw tracking channels : " << (int)data.numTrkChHw << '\n';
        sstr << " nr of channels in use      : " << (int)data.numTrkChUse << '\n';
        sstr << " Nr of config blocks        : " << (int)data.numConfigBlocks << "  (nr of sections=" << N << ")";
        sstr << "  Config Data :\n";
        logDebug(sstr.str());
    }

    for (std::size_t i = 0; i < N; i++)
    {
        GnssConfigStruct config;
        const auto n{8 * i};
        config.gnssId = get<decltype(config.gnssId)>(msg.begin() + n + 4);
        config.resTrkCh = get<decltype(config.resTrkCh)>(msg.begin() + n + 5);
        config.maxTrkCh = get<decltype(config.maxTrkCh)>(msg.begin() + n + 6);
        config.flags = get<decltype(config.flags)>(msg.begin() + n + 8);
        if (logLevel() == LogLevel::Debug)
        {
            std::stringstream sstr;
            sstr << "   " << i << ":   GNSS name : ";
            sstr << std::string(Gnss::Id::name[std::clamp(
                static_cast<int>(config.gnssId), static_cast<int>(Gnss::Id::first), static_cast<int>(Gnss::Id::last))]);
            sstr << '\n';
            sstr << "      reserved (min)tracking channels  : " << (int)config.resTrkCh << '\n';
            sstr << "      max nr of tracking channels used : " << (int)config.maxTrkCh << '\n';
            sstr << "      flags  : 0x" << std::hex << (int)config.flags << "\n";
            logDebug(sstr.str());
        }
        data.configs.push_back(std::move(config));
    }
    return data;
}

auto MessageProcessor::UBXCfgMSG(const std::string &msg) -> std::optional<UbxEvent>
{
    // caution: the message id is stored in the first two bytes of the data array
    // with message class in the first and message id in the second byte.
    // So, reading the 16-bit message with one operation, the endianness is big
    // and not little (as the default would be using get)
    CfgMsg data;
    data.msgID = get<std::uint16_t, endian::big>(msg.begin());
    data.rate = get<std::uint8_t>(msg.begin() + 2 + s_default_target);

    return data;
}

auto MessageProcessor::UBXMonRx(const std::string &msg) -> std::optional<UbxEvent>
{
    MonRx data;

    for (std::size_t target = 0; target < s_nr_targets; target++)
    {
        data.pending[target] = get<uint16_t>(msg.begin() + 2 * target);
        data.usage[target] = get<std::uint8_t>(msg.begin() + target + 12);
        data.peakUsage[target] = get<std::uint8_t>(msg.begin() + target + 18);
        data.tUsage += data.usage[target];
        data.tPeakUsage += data.peakUsage[target];
    }

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << std::setfill(' ') << std::setw(3);
        sstr << "*** UBX-MON-RXBUF message:" << '\n';
        sstr << " global RX buf usage      : " << (int)data.tUsage << " %" << '\n';
        sstr << " global RX buf peak usage : " << (int)data.tPeakUsage << " %" << '\n';
        sstr << " RX buf usage for target      : ";
        for (std::size_t i = 0; i < s_nr_targets; i++)
        {
            sstr << "    (" << i << ") " << std::setw(3) << (int)data.usage[i];
        }
        sstr << '\n';
        sstr << " RX buf peak usage for target : ";
        for (std::size_t i = 0; i < s_nr_targets; i++)
        {
            sstr << "    (" << i << ") " << std::setw(3) << (int)data.peakUsage[i];
        }
        sstr << '\n';
        sstr << " RX bytes pending for target  : ";
        for (std::size_t i = 0; i < s_nr_targets; i++)
        {
            sstr << "    (" << i << ") " << std::setw(3) << data.pending[i];
        }
        sstr << "\n";
        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXMonTx(const std::string &msg) -> std::optional<UbxEvent>
{
    MonTx data;
    for (std::size_t target = 0; target < s_nr_targets; target++)
    {
        data.pending[target] = get<uint16_t>(msg.begin() + 2 * target);
        data.usage[target] = get<std::uint8_t>(msg.begin() + target + 12);
        data.peakUsage[target] = get<std::uint8_t>(msg.begin() + target + 18);
    }

    data.tUsage = get<std::uint8_t>(msg.begin() + 24);
    data.tPeakUsage = get<std::uint8_t>(msg.begin() + 25);

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << std::setfill(' ') << std::setw(3);
        sstr << "*** UBX-MON-TXBUF message:" << '\n';
        sstr << " global TX buf usage      : " << (int)data.tUsage << " %" << '\n';
        sstr << " global TX buf peak usage : " << (int)data.tPeakUsage << " %" << '\n';
        sstr << " TX buf usage for target      : ";
        for (std::size_t i = 0; i < s_nr_targets; i++)
        {
            sstr << "    (" << i << ") " << std::setw(3) << (int)data.usage[i];
        }
        sstr << '\n';
        sstr << " TX buf peak usage for target : ";
        for (std::size_t i = 0; i < s_nr_targets; i++)
        {
            sstr << "    (" << i << ") " << std::setw(3) << (int)data.peakUsage[i];
        }
        sstr << '\n';
        sstr << " TX bytes pending for target  : ";
        for (std::size_t i = 0; i < s_nr_targets; i++)
        {
            sstr << "    (" << i << ") " << std::setw(3) << data.pending[i];
        }
        sstr << "\n";
        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXMonHW(const std::string &msg) -> std::optional<UbxEvent>
{
    GnssMonHwStruct data;
    // parse all fields
    // noise
    data.noisePerMS = get<std::uint16_t>(msg.begin() + 16);

    // agc
    data.agcCnt = get<std::uint16_t>(msg.begin() + 18);

    data.antStatus = get<std::uint8_t>(msg.begin() + 20);
    data.antPower = get<std::uint8_t>(msg.begin() + 21);
    data.flags = get<std::uint8_t>(msg.begin() + 22);
    data.jamInd = get<std::uint8_t>(msg.begin() + 45);

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX-MON-HW message:" << '\n';
        sstr << " noise            : " << data.noisePerMS << " dBc" << '\n';
        sstr << " agcCnt (0..8192) : " << data.agcCnt << '\n';
        sstr << " antenna status   : " << (int)data.antStatus << '\n';
        sstr << " antenna power    : " << (int)data.antPower << '\n';
        sstr << " jamming indicator: " << (int)data.jamInd << '\n';
        sstr << " flags             : ";
        for (int i = 7; i >= 0; i--)
            if (data.flags & 1 << i)
                sstr << i;
            else
                sstr << "-";
        sstr << '\n';
        sstr << "   RTC calibrated   : " << std::string((data.flags & 1) ? "yes" : "no") << '\n';
        sstr << "   safe boot        : " << std::string((data.flags & 2) ? "yes" : "no") << '\n';
        sstr << "   jamming state    : " << std::dec << (int)((data.flags & 0x0c) >> 2) << '\n';
        sstr << "   Xtal absent      : " << std::string((data.flags & 0x10) ? "yes" : "no");
        sstr << "\n";
        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXMonHW2(const std::string &msg) -> std::optional<UbxEvent>
{
    GnssMonHw2Struct data;
    // parse all fields
    // I/Q offset and magnitude information of front-end
    auto ofsI{get<int8_t>(msg.begin())};
    auto magI{get<std::uint8_t>(msg.begin() + 1)};
    auto ofsQ{get<int8_t>(msg.begin() + 2)};
    auto magQ{get<std::uint8_t>(msg.begin() + 3)};

    auto cfgSrc{get<std::uint8_t>(msg.begin() + 4)};
    // auto lowLevCfg { get<uint32_t>(msg.begin() + 8) };
    auto postStatus{get<uint32_t>(msg.begin() + 20)};

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX-MON-HW2 message:" << '\n';
        sstr << " I offset         : " << (int)ofsI << '\n';
        sstr << " I magnitude      : " << (int)magI << '\n';
        sstr << " Q offset         : " << (int)ofsQ << '\n';
        sstr << " Q magnitude      : " << (int)magQ << '\n';
        sstr << " config source    : " << std::hex << (int)cfgSrc << '\n';
        sstr << " POST status word : " << std::hex << postStatus << std::dec << '\n';
        logDebug(sstr.str());
    }
    return data;
}


// Parses strings like:
// "9.10"
// "10.0"
// "18"
// "  23.01 "
// Returns std::nullopt on invalid input.
auto MessageProcessor::getProtVersion(std::string_view text) -> std::optional<Version>
{
    // trim whitespace
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
        text.remove_prefix(1);

    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
        text.remove_suffix(1);

    if (text.empty())
        return std::nullopt;

    Version v{};

    auto dotPos = text.find('.');

    if (dotPos == std::string_view::npos)
    {
        // major only
        auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), v.major);
        if (ec != std::errc{} || ptr != text.data() + text.size())
            return std::nullopt;

        v.minor = 0;
        return v;
    }

    auto majorPart = text.substr(0, dotPos);
    auto minorPart = text.substr(dotPos + 1);

    if (majorPart.empty() || minorPart.empty())
        return std::nullopt;

    auto [ptr1, ec1] =
        std::from_chars(majorPart.data(), majorPart.data() + majorPart.size(), v.major);

    auto [ptr2, ec2] =
        std::from_chars(minorPart.data(), minorPart.data() + minorPart.size(), v.minor);

    if (ec1 != std::errc{} || ptr1 != majorPart.data() + majorPart.size())
        return std::nullopt;

    if (ec2 != std::errc{} || ptr2 != minorPart.data() + minorPart.size())
        return std::nullopt;

    return v;
}

auto MessageProcessor::UBXMonVer(const std::string &msg) -> std::optional<UbxEvent>
{
    // parse all fields
    GpsVersion data;

    for (int i = 0; msg[i] != 0 && i < 30; i++)
    {
        data.swString += (char)msg[i];
    }
    for (int i = 30; msg[i] != 0 && i < 40; i++)
    {
        data.hwString += (char)msg[i];
    }

    std::stringstream sstr;
    std::string::size_type i = 0;
    while (i != std::string::npos && i < msg.size())
    {
        std::string s = msg.substr(i, msg.find((char)0x00, i + 1) - i + 1);
        while (s.size() && s[0] == 0x00)
        {
            s.erase(0, 1);
        }
        if (s.size())
        {
            size_t n = s.find("PROTVER", 0);
            if (n != std::string::npos)
            {
                std::string str = s.substr(7, s.size() - 7);
                while (str.size() && !isdigit(str[0]))
                    str.erase(0, 1);
                while (str.size() &&
                       (str[str.size() - 1] == ' ' || !std::isgraph(static_cast<unsigned char>(str[str.size() - 1]))))
                    str.erase(str.size() - 1, 1);
                data.prot = str;
                auto version = getProtVersion(str);
                if (logLevel() == LogLevel::Debug && version.has_value())
                {
                    sstr << "caught PROTVER string: '" << str << "'\n";
                    sstr << "ver: " + std::to_string(version.value().major) + "." + std::to_string(version.value().minor) + "\n";
                }
            }
        }
        i = msg.find((char)0x00, i + 1);
    }

    if (logLevel() == LogLevel::Debug)
    {
        sstr << "*** UBX-MON-VER message:" << '\n';
        sstr << " sw version  : " << data.swString << '\n';
        sstr << " hw version  : " << data.hwString << '\n';
        logDebug(sstr.str());
    }
    gpsVersion = data;
    return data;
}

auto MessageProcessor::UBXTimTP(const std::string &msg) -> std::optional<UbxEvent>
{
    TimTP data;
    // parse all fields
    // TP time of week, ms
    data.towMS = get<std::uint32_t>(msg.begin());
    // TP time of week, sub ms
    data.towSubMS = get<std::uint32_t>(msg.begin() + 4);
    // quantization error
    data.qErr = get<std::int32_t>(msg.begin() + 8);
    // week number
    data.week = get<std::uint16_t>(msg.begin() + 12);
    // flags
    data.flags = get<std::uint8_t>(msg.begin() + 14);
    // ref info
    data.refInfo = get<std::uint8_t>(msg.begin() + 14);

    double sr = data.towMS / 1000.;
    sr = sr - data.towMS / 1000;

    if (logLevel() == LogLevel::Debug)
    {
        std::stringstream sstr;
        sstr << "*** UBX-TIM-TP message:" << '\n';
        sstr << " tow s            : " << std::dec << data.towMS / 1000. << " s" << '\n';
        sstr << " tow sub s        : " << std::dec << data.towSubMS << " = ";
        sstr << (long int)(sr * 1e9 + data.towSubMS + 0.5) << " ns" << '\n';
        sstr << " quantization err : " << std::dec << data.qErr << " ps" << '\n';
        sstr << " week nr : " << std::dec << data.week << '\n';
        sstr << " *flags            : ";
        for (int i = 7; i >= 0; i--)
        {
            if (data.flags & 1 << i)
            {
                sstr << i;
            }
            else
            {
                sstr << "-";
            }
        }
        sstr << '\n';
        sstr << "  time base     : " << std::string(((data.flags & 1) ? "UTC" : "GNSS")) << '\n';
        sstr << "  UTC available : " << std::string((data.flags & 2) ? "yes" : "no") << '\n';
        sstr << "  (T)RAIM info  : " << (int)((data.flags & 0x0c) >> 2) << '\n';
        sstr << " *refInfo          : ";
        for (int i = 7; i >= 0; i--)
            if (data.refInfo & 1 << i)
                sstr << i;
            else
                sstr << "-";
        sstr << '\n';
        std::string gnssRef;
        switch (data.refInfo & 0x0f)
        {
        case 0:
            gnssRef = "GPS";
            break;
        case 1:
            gnssRef = "GLONASS";
            break;
        case 2:
            gnssRef = "BeiDou";
            break;
        default:
            gnssRef = "unknown";
        }
        sstr << "  GNSS reference : " << gnssRef << '\n';
        std::string utcStd;
        switch ((data.refInfo & 0xf0) >> 4)
        {
        case 0:
            utcStd = "n/a";
            break;
        case 1:
            utcStd = "CRL";
            break;
        case 2:
            utcStd = "NIST";
            break;
        case 3:
            utcStd = "USNO";
            break;
        case 4:
            utcStd = "BIPM";
            break;
        case 5:
            utcStd = "EU";
            break;
        case 6:
            utcStd = "SU";
            break;
        default:
            utcStd = "unknown";
        }
        sstr << "  UTC standard  : " << utcStd << "\n";
        logDebug(sstr.str());
    }
    return data;
}

auto MessageProcessor::UBXTimTM2(const std::string &msg) -> std::optional<UbxEvent>
{
    // parse all fields
    // channel
    auto ch{get<std::uint8_t>(msg.begin())};
    // flags
    auto flags{get<std::uint8_t>(msg.begin() + 1)};
    // rising edge counter
    auto count{get<uint16_t>(msg.begin() + 2)};
    // week number of last rising edge
    auto wnR{get<uint16_t>(msg.begin() + 4)};
    // week number of last falling edge
    auto wnF{get<uint16_t>(msg.begin() + 6)};
    // time of week of rising edge, ms
    auto towMsR{get<uint32_t>(msg.begin() + 8)};
    // time of week of rising edge, sub ms
    auto towSubMsR{get<uint32_t>(msg.begin() + 12)};
    // time of week of falling edge, ms
    auto towMsF{get<uint32_t>(msg.begin() + 16)};
    // time of week of falling edge, sub ms
    auto towSubMsF{get<uint32_t>(msg.begin() + 20)};
    // accuracy estimate
    auto accEst{get<uint32_t>(msg.begin() + 24)};

    double sr = towMsR / 1000.;
    sr = sr - towMsR / 1000;
    double sf = towMsF / 1000.;
    sf = sf - towMsF / 1000;

    // meaning of columns:
    // 0d 03 - signature of TIM-TM2 message
    // ch, week nr, second in current week (rising), ns of timestamp in current second (rising),
    // second in current week (falling), ns of timestamp in current second (falling),
    // accuracy (ns), rising edge counter, rising/falling edge (1/0), time valid (GNSS fix)

    std::stringstream sstr;
    if (logLevel() == LogLevel::Debug)
    {
        sstr << "*** UBX-TimTM2 message:" << '\n';
        sstr << " channel         : " << std::dec << (int)ch << '\n';
        sstr << " rising edge ctr : " << std::dec << count << '\n';
        sstr << " * last rising edge:" << '\n';
        sstr << "    week nr        : " << std::dec << wnR << '\n';
        sstr << "    tow s          : " << std::dec << towMsR / 1000. << " s" << '\n';
        sstr << "    tow sub s     : " << std::dec << towSubMsR << " = ";
        sstr << (long int)(sr * 1e9 + towSubMsR) << "ns" << '\n';
        sstr << " * last falling edge:" << '\n';
        sstr << "    week nr        : ";
        sstr << std::dec << wnF << '\n';
        sstr << "    tow s          : " << std::dec << towMsF / 1000. << " s" << '\n';
        sstr << "    tow sub s: " << std::dec << towSubMsF << " = ";
        sstr << (long int)(sf * 1e9 + towSubMsF) << " ns" << '\n';
        sstr << " accuracy est: " << std::dec << accEst << " ns" << '\n';
        sstr << " flags             : ";
        for (int i = 7; i >= 0; i--)
        {
            if (flags & 1 << i)
            {
                sstr << i;
            }
            else
            {
                sstr << "-";
            }
        }
        sstr << '\n';
        sstr << "   mode                 : ";
        sstr << std::string((flags & 1) ? "single" : "running") << '\n';
        sstr << "   run                  : " << std::string((flags & 2) ? "armed" : "stopped");
        sstr << '\n';
        sstr << "   new rising edge      : ";
        sstr << std::string((flags & 0x80) ? "yes" : "no") << '\n';
        sstr << "   new falling edge     : " << std::string((flags & 0x04) ? "yes" : "no") << '\n';
        sstr << "   UTC available        : " << std::string((flags & 0x20) ? "yes" : "no") << '\n';
        sstr << "   time valid (GNSS fix): " << std::string((flags & 0x40) ? "yes" : "no") << '\n';
        std::string timeBase;
        switch ((flags & 0x18) >> 3)
        {
        case 0:
            timeBase = "receiver time";
            break;
        case 1:
            timeBase = "GNSS";
            break;
        case 2:
            timeBase = "UTC";
            break;
        default:
            timeBase = "unknown";
        }
        sstr << "   time base            : " << timeBase << "\n";
        logDebug(sstr.str());
        sstr.clear();

        if (flags & 0x80)
        {
            // if new rising edge
            sstr << unixtime_from_gps(wnR, towMsR / 1000, (long int)(sr * 1e9 + towSubMsR));
        }
        else
        {
            sstr << ".................... ";
        }
        if (flags & 0x04)
        {
            // if new falling edge
            sstr << unixtime_from_gps(wnF, towMsF / 1000, (long int)(sr * 1e9 + towSubMsF));
        }
        else
        {
            sstr << ".................... ";
        }
        sstr << accEst << " " << count << " " << ((flags & 0x40) >> 6) << " " << std::setfill('0') << std::setw(1)
            << ((flags & 0x18) >> 3) << " " << ((flags & 0x20) >> 5);
        logDebug(sstr.str());
    }


    struct timespec ts_r = unixtime_from_gps(wnR, towMsR / 1000, (long int)(sr * 1e9 + towSubMsR));
    struct timespec ts_f = unixtime_from_gps(wnF, towMsF / 1000, (long int)(sf * 1e9 + towSubMsF));

    struct gpsTimestamp ts;
    ts.rising_time = ts_r;
    ts.falling_time = ts_f;
    ts.valid = (flags & 0x40);
    ts.channel = ch;

    ts.counter = count;
    ts.accuracy_ns = accEst;

    ts.rising = ts.falling = false;
    if (flags & 0x80)
    {
        // new rising edge detected
        ts.rising = true;
    }
    if (flags & 0x04)
    {
        // new falling edge detected
        ts.falling = true;
    }

    static int64_t lastPulseLength = 0;

    UbxTimeMarkStruct tm;
    tm.rising = ts.rising_time;
    tm.falling = ts.falling_time;
    tm.risingValid = ts.rising;
    tm.fallingValid = ts.falling;
    tm.accuracy_ns = accEst;
    tm.valid = ts.valid;
    tm.timeBase = (flags & 0x18) >> 3;
    tm.utcAvailable = flags & 0x20;
    tm.flags = flags;
    tm.evtCounter = count;

    // try to recover the timestamp, if one edge is missing
    if (!tm.risingValid && tm.fallingValid)
    {
        tm.rising.tv_sec = tm.falling.tv_sec - lastPulseLength / 1000000000;
        tm.rising.tv_nsec = tm.falling.tv_nsec - lastPulseLength % 1000000000;
        if (tm.rising.tv_nsec >= 1000000000)
        {
            tm.rising.tv_sec += 1;
            tm.rising.tv_nsec -= 1000000000;
        }
        else if (tm.rising.tv_nsec < 0)
        {
            tm.rising.tv_sec -= 1;
            tm.rising.tv_nsec += 1000000000;
        }
    }
    else if (!tm.fallingValid && tm.risingValid)
    {
        tm.falling.tv_sec = tm.rising.tv_sec + lastPulseLength / 1000000000;
        tm.falling.tv_nsec = tm.rising.tv_nsec + lastPulseLength % 1000000000;
        if (tm.falling.tv_nsec >= 1000000000)
        {
            tm.falling.tv_sec += 1;
            tm.falling.tv_nsec -= 1000000000;
        }
    }
    else if (!tm.fallingValid && !tm.risingValid)
    {
        // nothing to recover here; ignore the event
    }
    else
    {
        // the normal case, i.e. both edges are valid
        // calculate the pulse length in this case
        int64_t dts = (tm.falling.tv_sec - tm.rising.tv_sec) * 1.0e9;
        dts += (tm.falling.tv_nsec - tm.rising.tv_nsec);
        if (dts > 0 && dts < 1000000)
            lastPulseLength = dts;
    }

    return tm;
}

/*
void QtSerialUblox::setGnssConfig(const std::vector<GnssConfigStruct>& gnssConfigs)
{
    const std::size_t N = gnssConfigs.size();
    unsigned char* data { static_cast<unsigned char*>(calloc(sizeof(unsigned char), 4 + 8 * N)) };
    data[0] = 0;
    data[1] = 0;
    data[2] = 0xff;
    data[3] = static_cast<std::uint8_t>(N);

    for (std::size_t i { 0 }; i < N; i++) {
        uint32_t flags = gnssConfigs[i].flags;
        if (getProtVersion() >= 15.0) {
            flags |= 0x0001 << 16;
            if (gnssConfigs[i].gnssId == 5)
                flags |= 0x0004 << 16;
        }
        data[4 + 8 * i] = gnssConfigs[i].gnssId;
        data[5 + 8 * i] = gnssConfigs[i].resTrkCh;
        data[6 + 8 * i] = gnssConfigs[i].maxTrkCh;
        data[8 + 8 * i] = flags & 0xff;
        data[9 + 8 * i] = (flags >> 8) & 0xff;
        data[10 + 8 * i] = (flags >> 16) & 0xff;
        data[11 + 8 * i] = (flags >> 24) & 0xff;
    }

    enqueueMsg(UBX_MSG::CFG_GNSS, toStdString(data, static_cast<int>(4 + 8 * N)));
    free(data);
}



void QtSerialUblox::setDynamicModel(UbxDynamicModel model)
{
    unsigned char* buf { static_cast<unsigned char*>(calloc(sizeof(unsigned char), 36)) };
    buf[0] = 0x01;
    buf[1] = 0x00;
    buf[2] = static_cast<std::uint8_t>(model); // dyn Model
    enqueueMsg(UBX_MSG::CFG_NAV5, toStdString(buf, 36));
    free(buf);
}
*/