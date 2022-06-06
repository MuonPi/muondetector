#include "qtserialublox.h"
#include "utility/unixtime_from_gps.h"
#include <custom_io_operators.h>

#include <muondetector_structs.h>
#include <ublox_messages.h>

#include <QThread>
#include <cctype>
#include <iomanip>
#include <sstream>

template <class T, class = void>
struct is_iterator : std::false_type {
};

template <class T>
struct is_iterator<T, std::void_t<typename std::iterator_traits<T>::iterator_category>> : std::true_type {
};

enum class endian : bool {
    big,
    little
};

template <typename T, endian Endian = endian::little, typename It, std::enable_if_t<std::is_integral<T>::value, bool> = true, std::enable_if_t<is_iterator<It>::value, bool> = true>
[[nodiscard]] auto get(const It& start) -> T
{
    const auto& end { start + sizeof(T) };
    T value { 0 };
    std::size_t shift { (Endian == endian::little) ? 0 : (sizeof(T) - 1) * 8 };
    for (auto it = start; it != end; it++) {
        value += static_cast<T>(*it) << shift;
        if (Endian == endian::little) {
            shift += 8;
        } else {
            shift -= 8;
        }
    }
    return value;
}

template <typename T, endian Endian = endian::little, typename It, std::enable_if_t<std::is_integral<T>::value, bool> = true, std::enable_if_t<is_iterator<It>::value, bool> = true>
void put(const It& start, const T& value)
{
    const auto& end { start + sizeof(T) };
    std::size_t shift { (Endian == endian::little) ? 0 : (sizeof(T) - 1) * 8 };
    for (auto it = start; it != end; it++) {
        *it = static_cast<std::uint8_t>((value >> shift) & 0xff);
        if (Endian == endian::little) {
            shift += 8;
        } else {
            shift -= 8;
        }
    }
}

void QtSerialUblox::processMessage(const UbxMessage& msg)
{
    static const std::map<std::uint8_t, const char*> ubx_class_names {
        { 0x01, "UBX-NAV" }, { 0x02, "UBX-RXM" }, { 0x04, "UBX-INF" }, { 0x05, "UBX-ACK" }, { 0x06, "UBX-CFG" }, { 0x09, "UBX-UPD" }, { 0x10, "UBX-ESF" }, { 0x13, "UBX-MGA" }, { 0x0a, "UBX-MON" }, { 0x0b, "UBX-AID" }, { 0x0d, "UBX-TIM" }, { 0x21, "UBX-LOG" }, { 0x27, "UBX-SEC" }, { 0x28, "UBX-HNR" }
    };

    const std::map<uint16_t, std::pair<std::function<void()>, std::string>> handler {
        { UBX_MSG::NAV_STATUS, std::make_pair([&] { UBXNavStatus(msg.payload()); }, "UBX-NAV-STATUS") }, { UBX_MSG::NAV_DOP, std::make_pair([&] { UBXNavDOP(msg.payload()); }, "UBX-NAV-DOP") }, { UBX_MSG::NAV_TIMEGPS, std::make_pair([&] { UBXNavTimeGPS(msg.payload()); }, "UBX-NAV-TIMEGPS") }, { UBX_MSG::NAV_TIMEUTC, std::make_pair([&] { UBXNavTimeUTC(msg.payload()); }, "UBX-NAV-TIMEUTC") }, { UBX_MSG::NAV_CLOCK, std::make_pair([&] { UBXNavClock(msg.payload()); }, "UBX-NAV-CLOCK") }, { UBX_MSG::NAV_SVINFO, std::make_pair([&] { UBXNavSVinfo(msg.payload(), true); }, "UBX-NAV-SVINFO") }, { UBX_MSG::NAV_SAT, std::make_pair([&] { UBXNavSat(msg.payload(), true); }, "UBX-NAV-SAT") }, { UBX_MSG::NAV_POSLLH, std::make_pair([&] { UBXNavPosLLH(msg.payload()); }, "UBX-NAV-POSLLH") }

        ,
        { UBX_MSG::CFG_ANT, std::make_pair([&] { UBXCfgAnt(msg.payload()); }, "UBX-CFG-ANT") }, { UBX_MSG::CFG_NAVX5, std::make_pair([&] { UBXCfgNavX5(msg.payload()); }, "UBX-CFG-NAVX5") }, { UBX_MSG::CFG_NAV5, std::make_pair([&] { UBXCfgNav5(msg.payload()); }, "UBX-CFG-NAV5") }, { UBX_MSG::CFG_TP5, std::make_pair([&] { UBXCfgTP5(msg.payload()); }, "UBX-CFG-TP5") }, { UBX_MSG::CFG_GNSS, std::make_pair([&] { UBXCfgGNSS(msg.payload()); }, "UBX-CFG-GNSS") }, { UBX_MSG::CFG_MSG, std::make_pair([&] { UBXCfgMSG(msg.payload()); }, "UBX-CFG-MSG") }

        ,
        { UBX_MSG::MON_RXBUF, std::make_pair([&] { UBXMonRx(msg.payload()); }, "UBX-MON-RXBUF") }, { UBX_MSG::MON_TXBUF, std::make_pair([&] { UBXMonTx(msg.payload()); }, "UBX-MON-TXBUF") }, { UBX_MSG::MON_HW, std::make_pair([&] { UBXMonHW(msg.payload()); }, "UBX-MON-HW") }, { UBX_MSG::MON_HW2, std::make_pair([&] { UBXMonHW2(msg.payload()); }, "UBX-MON-HW2") }, { UBX_MSG::MON_VER, std::make_pair([&] { UBXMonVer(msg.payload()); }, "UBX-MON-VER") }

        ,
        { UBX_MSG::TIM_TP, std::make_pair([&] { UBXTimTP(msg.payload()); }, "UBX-TIM-TP") }, { UBX_MSG::TIM_TM2, std::make_pair([&] { UBXTimTM2(msg.payload()); }, "UBX-TIM-TM2") }
    };

    uint8_t classID = msg.class_id();
    uint8_t messageID = msg.message_id();

    if (handler.count(msg.full_id()) > 0) {
        const auto& [handle, name] = handler.at(msg.full_id());
        handle();
        if (verbose > 2) {
            std::stringstream tempStream {};
            tempStream << "received " << name << " message (0x" << std::hex << std::setfill('0') << std::setw(2) << (int)classID
                       << " 0x" << std::hex << (int)messageID << ")\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
    } else if (classID == 0x05) {
        if (msg.payload().size() < 2) {
            emit toConsole("received UBX-ACK message but data is corrupted\n");
            return;
        }
        if (!msgWaitingForAck) {
            if (verbose > 1) {
                std::stringstream tempStream {};
                tempStream << "received ACK message but no message is waiting for Ack (msgID: 0x";
                tempStream << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[0] << " 0x"
                           << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[1] << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            return;
        }
        if (verbose > 3) {
            std::stringstream tempStream {};
            if (messageID == 1) {
                tempStream << "received UBX-ACK-ACK message about msgID: 0x";
            } else {
                tempStream << "received UBX-ACK-NACK message about msgID: 0x";
            }
            tempStream << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[0] << " 0x"
                       << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[1] << "\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        auto ackedMsgID = (uint16_t)(msg.payload()[0]) << 8U | msg.payload()[1];
        if (ackedMsgID != msgWaitingForAck->full_id()) {
            if (verbose > 2) {
                std::stringstream tempStream {};
                tempStream << "received unexpected UBX-ACK message about msgID: 0x";
                tempStream << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[0] << " 0x"
                           << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[1] << "\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            return;
        }
        if (messageID == 0x00 && msgWaitingForAck) {
            emit UBXReceivedAckNak(msgWaitingForAck->full_id(),
                (uint16_t)(msgWaitingForAck->payload()[0]) << 8U
                    | msgWaitingForAck->payload()[1]);
        }
        ackTimer->stop();
        msgWaitingForAck.reset(nullptr);
        if (verbose > 3) {
            emit toConsole("processMessage: deleted message after ACK/NACK\n");
        }
        sendQueuedMsg();
    } else if (classID == 0x06) {
        if (verbose > 3) {
            std::stringstream tempStream {};
            tempStream << "received unhandled UBX-CFG message:";
            for (std::string::size_type i = 0; i < msg.payload().size(); i++) {
                tempStream << " 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)msg.payload()[i];
            }
            tempStream << "\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
    } else {
        if (verbose > 3) {
            std::stringstream tempStream {};
            if (ubx_class_names.count(classID) > 0) {
                tempStream << "received unhandled " << ubx_class_names.at(classID) << " message"
                           << " (0x" << std::hex << std::setfill('0') << std::setw(2) << (int)classID
                           << " 0x" << std::hex << (int)messageID << ")\n";
            } else {
                tempStream << "received unknown UBX message (0x" << std::hex << std::setfill('0') << std::setw(2) << (int)classID
                           << " 0x" << std::hex << (int)messageID << ")\n";
            }
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
    }
}

void QtSerialUblox::UBXTimTP(const std::string& msg)
{
    // parse all fields
    // TP time of week, ms
    auto towMS { get<uint32_t>(msg.begin()) };
    // TP time of week, sub ms
    auto towSubMS { get<uint32_t>(msg.begin() + 4) };
    // quantization error
    auto qErr { get<int32_t>(msg.begin() + 8) };

    emit gpsPropertyUpdatedInt32(qErr, TPQuantErr.updateAge(), 'e');
    TPQuantErr = qErr;
    // week number
    auto week { get<uint16_t>(msg.begin() + 12) };
    // flags
    auto flags { get<uint8_t>(msg.begin() + 14) };
    // ref info
    auto refInfo { get<uint8_t>(msg.begin() + 14) };

    double sr = towMS / 1000.;
    sr = sr - towMS / 1000;

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "*** UBX-TIM-TP message:" << '\n';
        tempStream << " tow s            : " << std::dec << towMS / 1000. << " s" << '\n';
        tempStream << " tow sub s        : " << std::dec << towSubMS << " = " << (long int)(sr * 1e9 + towSubMS + 0.5) << " ns" << '\n';
        tempStream << " quantization err : " << std::dec << qErr << " ps" << '\n';
        tempStream << " week nr          : " << std::dec << week << '\n';
        tempStream << " *flags            : ";
        for (int i = 7; i >= 0; i--)
            if (flags & 1 << i)
                tempStream << i;
            else
                tempStream << "-";
        tempStream << '\n';
        tempStream << "  time base     : " << std::string(((flags & 1) ? "UTC" : "GNSS")) << '\n';
        tempStream << "  UTC available : " << std::string((flags & 2) ? "yes" : "no") << '\n';
        tempStream << "  (T)RAIM info  : " << (int)((flags & 0x0c) >> 2) << '\n';
        tempStream << " *refInfo          : ";
        for (int i = 7; i >= 0; i--)
            if (refInfo & 1 << i)
                tempStream << i;
            else
                tempStream << "-";
        tempStream << '\n';
        std::string gnssRef;
        switch (refInfo & 0x0f) {
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
        tempStream << "  GNSS reference : " << gnssRef << '\n';
        std::string utcStd;
        switch ((refInfo & 0xf0) >> 4) {
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
        tempStream << "  UTC standard  : " << utcStd << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXTimTM2(const std::string& msg)
{
    // parse all fields
    // channel
    auto ch { get<uint8_t>(msg.begin()) };
    // flags
    auto flags { get<uint8_t>(msg.begin() + 1) };
    // rising edge counter
    auto count { get<uint16_t>(msg.begin() + 2) };
    // week number of last rising edge
    auto wnR { get<uint16_t>(msg.begin() + 4) };
    // week number of last falling edge
    auto wnF { get<uint16_t>(msg.begin() + 6) };
    // time of week of rising edge, ms
    auto towMsR { get<uint32_t>(msg.begin() + 8) };
    // time of week of rising edge, sub ms
    auto towSubMsR { get<uint32_t>(msg.begin() + 12) };
    // time of week of falling edge, ms
    auto towMsF { get<uint32_t>(msg.begin() + 16) };
    // time of week of falling edge, sub ms
    auto towSubMsF { get<uint32_t>(msg.begin() + 20) };
    // accuracy estimate
    auto accEst { get<uint32_t>(msg.begin() + 24) };

    double sr = towMsR / 1000.;
    sr = sr - towMsR / 1000;
    double sf = towMsF / 1000.;
    sf = sf - towMsF / 1000;

    // meaning of columns:
    // 0d 03 - signature of TIM-TM2 message
    // ch, week nr, second in current week (rising), ns of timestamp in current second (rising),
    // second in current week (falling), ns of timestamp in current second (falling),
    // accuracy (ns), rising edge counter, rising/falling edge (1/0), time valid (GNSS fix)

    std::stringstream tempStream;
    if (verbose > 2) {
        tempStream << "*** UBX-TimTM2 message:" << '\n';
        tempStream << " channel         : " << std::dec << (int)ch << '\n';
        tempStream << " rising edge ctr : " << std::dec << count << '\n';
        tempStream << " * last rising edge:" << '\n';
        tempStream << "    week nr        : " << std::dec << wnR << '\n';
        tempStream << "    tow s          : " << std::dec << towMsR / 1000. << " s" << '\n';
        tempStream << "    tow sub s     : " << std::dec << towSubMsR << " = " << (long int)(sr * 1e9 + towSubMsR) << " ns" << '\n';
        tempStream << " * last falling edge:" << '\n';
        tempStream << "    week nr        : " << std::dec << wnF << '\n';
        tempStream << "    tow s          : " << std::dec << towMsF / 1000. << " s" << '\n';
        tempStream << "    tow sub s      : " << std::dec << towSubMsF << " = " << (long int)(sf * 1e9 + towSubMsF) << " ns" << '\n';
        tempStream << " accuracy est      : " << std::dec << accEst << " ns" << '\n';
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--)
            if (flags & 1 << i)
                tempStream << i;
            else
                tempStream << "-";
        tempStream << '\n';
        tempStream << "   mode                 : " << std::string((flags & 1) ? "single" : "running") << '\n';
        tempStream << "   run                  : " << std::string((flags & 2) ? "armed" : "stopped") << '\n';
        tempStream << "   new rising edge      : " << std::string((flags & 0x80) ? "yes" : "no") << '\n';
        tempStream << "   new falling edge     : " << std::string((flags & 0x04) ? "yes" : "no") << '\n';
        tempStream << "   UTC available        : " << std::string((flags & 0x20) ? "yes" : "no") << '\n';
        tempStream << "   time valid (GNSS fix): " << std::string((flags & 0x40) ? "yes" : "no") << '\n';
        std::string timeBase;
        switch ((flags & 0x18) >> 3) {
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
        tempStream << "   time base            : " << timeBase << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }

    tempStream.clear();
    if (flags & 0x80) {
        // if new rising edge
        tempStream << unixtime_from_gps(wnR, towMsR / 1000, (long int)(sr * 1e9 + towSubMsR));
    } else {
        tempStream << ".................... ";
    }
    if (flags & 0x04) {
        // if new falling edge
        tempStream << unixtime_from_gps(wnF, towMsF / 1000, (long int)(sr * 1e9 + towSubMsF));
    } else {
        tempStream << ".................... ";
    }
    tempStream << accEst
               << " " << count
               << " " << ((flags & 0x40) >> 6)
               << " " << std::setfill('0') << std::setw(1) << ((flags & 0x18) >> 3)
               << " " << ((flags & 0x20) >> 5);
    if (verbose > 1) {
        emit toConsole(QString::fromStdString(tempStream.str()) + "\n");
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
    if (flags & 0x80) {
        // new rising edge detected
        ts.rising = true;
    }
    if (flags & 0x04) {
        // new falling edge detected
        ts.falling = true;
    }
    emit gpsPropertyUpdatedUint32(count, eventCounter.updateAge(), 'c');
    eventCounter = count;

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
    if (!tm.risingValid && tm.fallingValid) {
        tm.rising.tv_sec = tm.falling.tv_sec - lastPulseLength / 1000000000;
        tm.rising.tv_nsec = tm.falling.tv_nsec - lastPulseLength % 1000000000;
        if (tm.rising.tv_nsec >= 1000000000) {
            tm.rising.tv_sec += 1;
            tm.rising.tv_nsec -= 1000000000;
        } else if (tm.rising.tv_nsec < 0) {
            tm.rising.tv_sec -= 1;
            tm.rising.tv_nsec += 1000000000;
        }
    } else if (!tm.fallingValid && tm.risingValid) {
        tm.falling.tv_sec = tm.rising.tv_sec + lastPulseLength / 1000000000;
        tm.falling.tv_nsec = tm.rising.tv_nsec + lastPulseLength % 1000000000;
        if (tm.falling.tv_nsec >= 1000000000) {
            tm.falling.tv_sec += 1;
            tm.falling.tv_nsec -= 1000000000;
        }
    } else if (!tm.fallingValid && !tm.risingValid) {
        // nothing to recover here; ignore the event
    } else {
        // the normal case, i.e. both edges are valid
        // calculate the pulse length in this case
        int64_t dts = (tm.falling.tv_sec - tm.rising.tv_sec) * 1.0e9;
        dts += (tm.falling.tv_nsec - tm.rising.tv_nsec);
        if (dts > 0 && dts < 1000000)
            lastPulseLength = dts;
    }

    emit UBXReceivedTimeTM2(tm);
}

void QtSerialUblox::UBXNavSat(const std::string& msg, bool allSats)
{
    std::vector<GnssSatellite> satList;
    // UBX-NAV-SAT: satellite information
    // parse all fields
    // GPS time of week
    auto iTOW { get<uint32_t>(msg.begin()) };
    // version
    auto version { get<uint8_t>(msg.begin() + 4) };
    auto numSvs { get<uint8_t>(msg.begin() + 5) };

    const std::size_t N { (msg.size() - 8) / 12 };

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << std::setfill(' ') << std::setw(3);
        tempStream << "*** UBX-NAV-SAT message:" << '\n';
        tempStream << " iTOW          : " << iTOW / 1000 << " s" << '\n';
        tempStream << " version       : " << (int)version << '\n';
        tempStream << " Nr of sats    : " << (int)numSvs << "  (nr of sections=" << N << ")\n";
        tempStream << "   Sat Data :\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    uint8_t goodSats = 0;
    for (std::size_t i = 0; i < N; i++) {
        const std::size_t n { 8 + i * 12 };

        auto gnssId { get<uint8_t>(msg.begin() + n) };
        auto satId { get<uint8_t>(msg.begin() + n + 1) };
        auto cnr { get<uint8_t>(msg.begin() + n + 2) };
        auto elev { get<int8_t>(msg.begin() + n + 3) };
        auto azim { get<int16_t>(msg.begin() + n + 4) };
        auto prRes { static_cast<float>(get<int16_t>(msg.begin() + n + 6)) / 10.0F };
        auto flags { get<uint32_t>(msg.begin() + n + 8) };

        GnssSatellite sat(gnssId, satId, cnr, elev, azim, prRes, flags);

        if (sat.Cnr > 0) {
            goodSats++;
        }
        satList.push_back(sat);
    }
    if (!allSats) {
        sort(satList.begin(), satList.end(), GnssSatellite::sortByCnr);
        while (!satList.empty() && (satList.back().Cnr == 0)) {
            satList.pop_back();
        }
    }

    emit gpsPropertyUpdatedUint8(goodSats, nrSats.updateAge(), 's');
    nrSats = goodSats;

    if (verbose > 3) {
        std::string temp;
        GnssSatellite::PrintHeader(true);
        for (std::vector<GnssSatellite>::iterator it = satList.begin(); it != satList.end(); it++) {
            it->Print(distance(satList.begin(), it), false);
        }
        std::stringstream tempStream;
        tempStream << "   --------------------------------------------------------------------\n";
        tempStream << " Nr of avail sats : " << (int)goodSats << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }

    emit gpsPropertyUpdatedGnss(satList, m_satList.updateAge());
    m_satList = satList;
}

void QtSerialUblox::UBXNavSVinfo(const std::string& msg, bool allSats)
{
    std::vector<GnssSatellite> satList;
    // UBX-NAV-SVINFO: satellite information
    // parse all fields
    // GPS time of week
    auto iTOW { get<uint32_t>(msg.begin()) };
    auto numSvs { get<uint8_t>(msg.begin() + 4) };
    auto globFlags { get<uint8_t>(msg.begin() + 5) };

    const std::size_t N { (msg.size() - 8) / 12 };

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << std::setfill(' ') << std::setw(3);
        tempStream << "*** UBX-NAV-SVINFO message:" << '\n';
        tempStream << " iTOW          : " << iTOW / 1000 << " s" << '\n';
        tempStream << " global flags  : 0x" << std::hex << (int)globFlags << std::dec << '\n';
        tempStream << " Nr of sats    : " << (int)numSvs << "  (nr of sections=" << N << ")\n";
        tempStream << "   Sat Data :\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    uint8_t goodSats = 0;
    for (std::size_t i = 0; i < N; i++) {
        const std::size_t n { 8 + i * 12 };

        auto satId { get<uint8_t>(msg.begin() + n + 1) };
        auto flags { get<uint8_t>(msg.begin() + n + 2) };
        auto quality { get<uint8_t>(msg.begin() + n + 3) };
        auto cnr { get<uint8_t>(msg.begin() + n + 4) };
        auto elev { get<int8_t>(msg.begin() + n + 5) };
        auto azim { get<int16_t>(msg.begin() + n + 6) };
        auto prRes { static_cast<float>(get<int32_t>(msg.begin() + n + 8)) / 100.0F };

        bool used = false;
        if (flags & 0x01)
            used = true;
        uint8_t health = (flags >> 4 & 0x01);
        health += 1;
        uint8_t orbitSource = 0;
        if (flags & 0x04) {
            if (flags & 0x08)
                orbitSource = 1;
            else if (flags & 0x20)
                orbitSource = 2;
            else if (flags & 0x40)
                orbitSource = 3;
        }
        bool smoothed = (flags & 0x80);
        bool diffCorr = (flags & 0x02);

        int gnssId { [&satId] {
            if (satId < 33) {
                return 0;
            } else if (satId < 65) {
                return 3;
            } else if (satId < 97 || satId == 255) {
                return 6;
            } else if (satId < 159) {
                return 1;
            } else if (satId < 164) {
                return 3;
            } else if (satId < 183) {
                return 4;
            } else if (satId < 198) {
                return 5;
            } else if (satId < 247) {
                return 2;
            }
            return 7;
        }() };

        GnssSatellite sat(gnssId, satId, cnr, elev, azim, prRes,
            quality, health, orbitSource, used, diffCorr, smoothed);
        if (sat.Cnr > 0) {
            goodSats++;
        }
        satList.push_back(sat);
    }
    if (!allSats) {
        sort(satList.begin(), satList.end(), GnssSatellite::sortByCnr);
        while (!satList.empty() && (satList.back().Cnr == 0)) {
            satList.pop_back();
        }
    }

    emit gpsPropertyUpdatedUint8(goodSats, nrSats.updateAge(), 's');
    nrSats = goodSats;

    if (verbose > 3) {
        std::string temp;
        GnssSatellite::PrintHeader(true);
        for (std::vector<GnssSatellite>::iterator it = satList.begin(); it != satList.end(); it++) {
            it->Print(distance(satList.begin(), it), false);
        }
        std::stringstream tempStream;
        tempStream << "   --------------------------------------------------------------------\n";
        tempStream << " Nr of avail sats : " << goodSats << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }

    emit gpsPropertyUpdatedGnss(satList, m_satList.updateAge());
    m_satList = satList;
}

void QtSerialUblox::UBXCfgMSG(const std::string& msg)
{
    // caution: the message id is stored in the first two bytes of the data array
    // with message class in the first and message id in the second byte.
    // So, reading the 16-bit message with one operation, the endianness is big
    // and not little (as the default would be using get)
    auto msgID { get<uint16_t, endian::big>(msg.begin()) };
    auto rate { get<uint8_t>(msg.begin() + 2 + s_default_target) };

    emit UBXreceivedMsgRateCfg(msgID, rate);
}

void QtSerialUblox::UBXCfgGNSS(const std::string& msg)
{
    // UBX-CFG-GNSS: GNSS configuration
    // parse all fields
    // version
    // send:
    // "0,0,ff,1,6,5,ff,0,1,0,0,0"
    auto version { get<uint8_t>(msg.begin()) };
    auto numTrkChHw { get<uint8_t>(msg.begin() + 1) };
    auto numTrkChUse { get<uint8_t>(msg.begin() + 2) };
    auto numConfigBlocks { get<uint8_t>(msg.begin() + 3) };

    const std::size_t N { (msg.size() - 4) / 8 };

    if (verbose > 2) {
        std::stringstream tempStream;
        tempStream << "*** UBX CFG-GNSS message:" << '\n';
        tempStream << " version                    : " << (int)version << '\n';
        tempStream << " nr of hw tracking channels : " << (int)numTrkChHw << '\n';
        tempStream << " nr of channels in use      : " << (int)numTrkChUse << '\n';
        tempStream << " Nr of config blocks        : " << (int)numConfigBlocks
                   << "  (nr of sections=" << N << ")";
        tempStream << "  Config Data :\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    std::vector<GnssConfigStruct> configs;

    for (std::size_t i = 0; i < N; i++) {
        GnssConfigStruct config;
        const auto n { 8 * i };
        config.gnssId = get<decltype(config.gnssId)>(msg.begin() + n + 4);
        config.resTrkCh = get<decltype(config.resTrkCh)>(msg.begin() + n + 5);
        config.maxTrkCh = get<decltype(config.maxTrkCh)>(msg.begin() + n + 6);
        config.flags = get<decltype(config.flags)>(msg.begin() + n + 8);
        if (verbose > 2) {
            std::stringstream tempStream;
            tempStream << "   " << i << ":   GNSS name : "
                       << std::string(Gnss::Id::name[std::clamp(static_cast<int>(config.gnssId), static_cast<int>(Gnss::Id::first), static_cast<int>(Gnss::Id::last))]) << '\n';
            tempStream << "      reserved (min) tracking channels  : "
                       << (int)config.resTrkCh << '\n';
            tempStream << "      max nr of tracking channels used : "
                       << (int)config.maxTrkCh << '\n';
            tempStream << "      flags  : 0x" << std::hex << (int)config.flags << "\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        configs.push_back(config);
    }
    emit UBXReceivedGnssConfig(numTrkChHw, configs);
}

void QtSerialUblox::onSetGnssConfig(const std::vector<GnssConfigStruct>& gnssConfigs)
{
    const std::size_t N = gnssConfigs.size();
    unsigned char* data { static_cast<unsigned char*>(calloc(sizeof(unsigned char), 4 + 8 * N)) };
    data[0] = 0;
    data[1] = 0;
    data[2] = 0xff;
    data[3] = static_cast<uint8_t>(N);

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

void QtSerialUblox::UBXCfgNav5(const std::string& msg)
{
    // UBX CFG-NAV5: satellite information
    // parse all fields
    auto mask { get<uint16_t>(msg.begin()) };
    auto dynModel { get<uint8_t>(msg.begin() + 2) };
    auto fixMode { get<uint8_t>(msg.begin() + 3) };
    auto fixedAlt { get<int32_t>(msg.begin() + 4) };
    auto fixedAltVar { get<uint32_t>(msg.begin() + 8) };
    auto minElev { get<int8_t>(msg.begin() + 12) };
    auto cnoThreshNumSVs { get<uint8_t>(msg.begin() + 24) };
    auto cnoThresh { get<uint8_t>(msg.begin() + 25) };

    if (verbose > 2) {
        std::stringstream tempStream;
        tempStream << "*** UBX CFG-NAV5 message:" << '\n';
        tempStream << " mask               : " << std::hex << (int)mask << std::dec << '\n';
        tempStream << " dynamic model used : " << (int)dynModel << '\n';
        tempStream << " fixMode            : " << (int)fixMode << '\n';
        tempStream << " fixed Alt          : " << (double)fixedAlt * 0.01 << " m" << '\n';
        tempStream << " fixed Alt Var      : " << (double)fixedAltVar * 0.0001 << " m^2" << '\n';
        tempStream << " min elevation      : " << (int)minElev << " deg\n";
        tempStream << " cnoThresh required for fix : " << (int)cnoThresh << " dBHz\n";
        tempStream << " min nr of SVs having cnoThresh for fix : " << (int)cnoThreshNumSVs << '\n';
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
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

void QtSerialUblox::UBXNavStatus(const std::string& msg)
{
    // UBX-NAV_STATUS: RX status information
    // parse all fields
    auto iTOW { get<uint32_t>(msg.begin()) };
    auto gpsFix { get<uint8_t>(msg.begin() + 4) };
    auto flags { get<uint8_t>(msg.begin() + 5) };
    auto flags2 { get<uint8_t>(msg.begin() + 7) };
    auto ttff { get<uint32_t>(msg.begin() + 8) };
    auto msss { get<uint32_t>(msg.begin() + 12) };

    emit gpsPropertyUpdatedUint8(gpsFix, fix.updateAge(), 'f');
    fix = gpsFix;

    emit gpsPropertyUpdatedUint32(msss / 1000, std::chrono::duration<double>(0), 'u');

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "*** UBX NAV-STATUS message:" << '\n';
        tempStream << " iTOW             : " << iTOW / 1000 << " s" << '\n';
        tempStream << " gpsFix           : " << (int)gpsFix << '\n';
        tempStream << " time to first fix: " << (float)ttff / 1000. << " s" << '\n';
        tempStream << " uptime           : " << (float)msss / 1000. << " s" << '\n';
        tempStream << " flags            : " << std::hex << "0x" << (int)flags << std::dec << '\n';
        tempStream << " flags2           : " << std::hex << "0x" << (int)flags2 << std::dec << '\n';
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXNavPosLLH(const std::string& msg)
{
    GnssPosStruct pos {};
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
    geodeticPos = pos;
    emit gpsPropertyUpdatedGeodeticPos(geodeticPos());
}

void QtSerialUblox::UBXNavClock(const std::string& msg)
{
    // parse all fields
    // GPS time of week
    auto iTOW { get<uint32_t>(msg.begin()) };
    // clock bias
    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "clkB[0]=" << std::setfill('0') << std::setw(2) << std::hex << (int)msg[4] << '\n';
        tempStream << "clkB[1]=" << std::setfill('0') << std::setw(2) << std::hex << (int)msg[5] << '\n';
        tempStream << "clkB[2]=" << std::setfill('0') << std::setw(2) << std::hex << (int)msg[6] << '\n';
        tempStream << "clkB[3]=" << std::setfill('0') << std::setw(2) << std::hex << (int)msg[7];
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    auto clkB { get<int32_t>(msg.begin() + 4) };
    // clock drift
    auto clkD { get<int32_t>(msg.begin() + 8) };
    //mutex.lock();
    emit gpsPropertyUpdatedInt32(clkD, clkDrift.updateAge(), 'd');
    emit gpsPropertyUpdatedInt32(clkB, clkBias.updateAge(), 'b');
    clkDrift = clkD;
    clkBias = clkB;
    // time accuracy estimate
    auto tAcc { get<uint32_t>(msg.begin() + 12) };
    // freq accuracy estimate
    auto fAcc { get<uint32_t>(msg.begin() + 16) };

    emit gpsPropertyUpdatedUint32(tAcc, timeAccuracy.updateAge(), 'a');
    timeAccuracy = tAcc;
    timeAccuracy.lastUpdate = std::chrono::system_clock::now();

    emit gpsPropertyUpdatedUint32(fAcc, freqAccuracy.updateAge(), 'f');
    freqAccuracy = fAcc;
    freqAccuracy.lastUpdate = std::chrono::system_clock::now();
    // meaning of columns:
    // 01 22 - signature of NAV-CLOCK message
    // second in current week (s), clock bias (ns), clock drift (ns/s), time accuracy (ns), freq accuracy (ps/s)

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "*** UBX-NAV-CLOCK message:" << '\n';
        tempStream << " iTOW          : " << iTOW / 1000 << " s" << '\n';
        tempStream << " clock bias    : " << clkB << " ns" << '\n';
        tempStream << " clock drift   : " << clkD << " ns/s" << '\n';
        tempStream << " time accuracy : " << tAcc << " ns" << '\n';
        tempStream << " freq accuracy : " << fAcc << " ps/s\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXNavTimeGPS(const std::string& msg)
{
    // parse all fields
    // GPS time of week
    auto iTOW { get<uint32_t>(msg.begin()) };

    auto fTOW { get<int32_t>(msg.begin() + 4) };

    auto wnR { get<uint16_t>(msg.begin() + 8) };
    auto leapS { get<int8_t>(msg.begin() + 10) };
    auto flags { get<uint8_t>(msg.begin() + 11) };

    // time accuracy estimate
    auto tAcc { get<uint32_t>(msg.begin() + 12) };

    double sr = iTOW / 1000.;
    sr = sr - iTOW / 1000;

    // meaning of columns:
    // 01 20 - signature of NAV-TIMEGPS message
    // week nr, second in current week, ns of timestamp in current second,
    // nr of leap seconds wrt UTC, accuracy (ns)

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "*** UBX-NAV-TIMEGPS message:" << '\n';
        tempStream << " week nr       : " << std::dec << wnR << '\n';
        tempStream << " iTOW          : " << std::dec << iTOW << " ms = " << iTOW / 1000 << " s" << '\n';
        tempStream << " fTOW          : " << std::dec << fTOW << " = " << (long int)(sr * 1e9 + fTOW) << " ns" << '\n';
        tempStream << " leap seconds  : " << std::dec << (int)leapS << " s" << '\n';
        tempStream << " time accuracy : " << std::dec << tAcc << " ns" << '\n';
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--)
            if (flags & 1 << i)
                tempStream << i;
            else
                tempStream << "-";
        tempStream << '\n';
        tempStream << "   tow valid        : " << std::string((flags & 1) ? "yes" : "no") << '\n';
        tempStream << "   week valid       : " << std::string((flags & 2) ? "yes" : "no") << '\n';
        tempStream << "   leap sec valid   : " << std::string((flags & 4) ? "yes" : "no");
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    emit gpsPropertyUpdatedUint32(tAcc, timeAccuracy.updateAge(), 'a');
    timeAccuracy = tAcc;
    timeAccuracy.lastUpdate = std::chrono::system_clock::now();
    if (flags & 4) {
        leapSeconds = leapS;
    }

    struct timespec ts = unixtime_from_gps(wnR, iTOW / 1000, (long int)(sr * 1e9 + fTOW));
    std::stringstream tempStream;
    tempStream << ts.tv_sec << '.' << ts.tv_nsec << "\n";
    if (verbose > 1) {
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXNavTimeUTC(const std::string& msg)
{
    // parse all fields
    // GPS time of week
    auto iTOW { get<uint32_t>(msg.begin()) };

    // time accuracy estimate
    auto tAcc { get<uint32_t>(msg.begin() + 4) };

    emit gpsPropertyUpdatedUint32(tAcc, timeAccuracy.updateAge(), 'a');
    timeAccuracy = tAcc;
    timeAccuracy.lastUpdate = std::chrono::system_clock::now();

    auto nano { get<int32_t>(msg.begin() + 8) };

    auto year { get<uint16_t>(msg.begin() + 12) };

    uint16_t month = (int)msg[14];
    uint16_t day = (int)msg[15];
    uint16_t hour = (int)msg[16];
    uint16_t min = (int)msg[17];
    uint16_t sec = (int)msg[18];

    uint8_t flags = (int)msg[19];

    double sr = iTOW / 1000.;
    sr = sr - iTOW / 1000;

    // meaning of columns:
    // 01 21 - signature of NAV-TIMEUTC message
    // second in current week, year, month, day, hour, minute, seconds(+fraction)
    // accuracy (ns)

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "*** UBX-NAV-TIMEUTC message:" << '\n';
        tempStream << " iTOW           : " << iTOW << " ms = " << iTOW / 1000 << " s" << '\n';
        tempStream << " nano           : " << nano << " ns" << '\n';
        tempStream << " date y/m/d     : " << (int)year << "/" << (int)month << "/" << (int)day << '\n';
        tempStream << " UTC time h:m:s : " << std::setw(2) << std::setfill('0') << hour << ":" << (int)min << ":" << (double)(sec + nano * 1e-9) << '\n';
        tempStream << " time accuracy : " << tAcc << " ns" << '\n';
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--)
            if (flags & 1 << i)
                tempStream << i;
            else
                tempStream << "-";
        tempStream << '\n';
        tempStream << "   tow valid        : " << std::string((flags & 1) ? "yes" : "no") << '\n';
        tempStream << "   week valid       : " << std::string((flags & 2) ? "yes" : "no") << '\n';
        tempStream << "   UTC time valid   : " << std::string((flags & 4) ? "yes" : "no") << '\n';
        std::string utcStd;
        switch ((flags & 0xf0) >> 4) {
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
        tempStream << "   UTC standard  : " << utcStd << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXMonHW(const std::string& msg)
{
    // parse all fields
    // noise
    auto noisePerMS { get<uint16_t>(msg.begin() + 16) };
    noise = noisePerMS;

    // agc
    auto agcCnt { get<uint16_t>(msg.begin() + 18) };
    agc = agcCnt;

    auto antStatus { get<uint8_t>(msg.begin() + 20) };
    auto antPower { get<uint8_t>(msg.begin() + 21) };
    auto flags { get<uint8_t>(msg.begin() + 22) };
    auto jamInd { get<uint8_t>(msg.begin() + 45) };

    // meaning of columns:
    // 01 21 - signature of NAV-TIMEUTC message
    // second in current week, year, month, day, hour, minute, seconds(+fraction)
    // accuracy (ns)

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "*** UBX-MON-HW message:" << '\n';
        tempStream << " noise            : " << noisePerMS << " dBc" << '\n';
        tempStream << " agcCnt (0..8192) : " << agcCnt << '\n';
        tempStream << " antenna status   : " << (int)antStatus << '\n';
        tempStream << " antenna power    : " << (int)antPower << '\n';
        tempStream << " jamming indicator: " << (int)jamInd << '\n';
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--)
            if (flags & 1 << i)
                tempStream << i;
            else
                tempStream << "-";
        tempStream << '\n';
        tempStream << "   RTC calibrated   : " << std::string((flags & 1) ? "yes" : "no") << '\n';
        tempStream << "   safe boot        : " << std::string((flags & 2) ? "yes" : "no") << '\n';
        tempStream << "   jamming state    : " << std::dec << (int)((flags & 0x0c) >> 2) << '\n';
        tempStream << "   Xtal absent      : " << std::string((flags & 0x10) ? "yes" : "no");
        tempStream << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    emit gpsMonHW(GnssMonHwStruct { noisePerMS, agcCnt, antStatus, antPower, jamInd, flags });
}

void QtSerialUblox::UBXMonHW2(const std::string& msg)
{
    // parse all fields
    // I/Q offset and magnitude information of front-end
    auto ofsI { get<int8_t>(msg.begin()) };
    auto magI { get<uint8_t>(msg.begin() + 1) };
    auto ofsQ { get<int8_t>(msg.begin() + 2) };
    auto magQ { get<uint8_t>(msg.begin() + 3) };

    auto cfgSrc { get<uint8_t>(msg.begin() + 4) };
    /* auto lowLevCfg { get<uint32_t>(msg.begin() + 8) }; */
    auto postStatus { get<uint32_t>(msg.begin() + 20) };

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "*** UBX-MON-HW2 message:" << '\n';
        tempStream << " I offset         : " << (int)ofsI << '\n';
        tempStream << " I magnitude      : " << (int)magI << '\n';
        tempStream << " Q offset         : " << (int)ofsQ << '\n';
        tempStream << " Q magnitude      : " << (int)magQ << '\n';
        tempStream << " config source    : " << std::hex << (int)cfgSrc << '\n';
        tempStream << " POST status word : " << std::hex << postStatus << std::dec << '\n';
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    emit gpsMonHW2(GnssMonHw2Struct { ofsI, ofsQ, magI, magQ, cfgSrc });
}

void QtSerialUblox::UBXMonVer(const std::string& msg)
{
    // parse all fields
    std::string hwString = "";
    std::string swString = "";

    for (int i = 0; msg[i] != 0 && i < 30; i++) {
        swString += (char)msg[i];
    }
    for (int i = 30; msg[i] != 0 && i < 40; i++) {
        hwString += (char)msg[i];
    }

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "*** UBX-MON-VER message:" << '\n';
        tempStream << " sw version  : " << swString << '\n';
        tempStream << " hw version  : " << hwString << '\n';
        emit toConsole(QString::fromStdString(tempStream.str()));
    }

    std::vector<std::string> result;
    std::string::size_type i = 0;
    while (i != std::string::npos && i < msg.size()) {
        std::string s = msg.substr(i, msg.find((char)0x00, i + 1) - i + 1);
        while (s.size() && s[0] == 0x00) {
            s.erase(0, 1);
        }
        if (s.size()) {
            result.push_back(s);
            size_t n = s.find("PROTVER", 0);
            if (n != std::string::npos) {
                std::string str = s.substr(7, s.size() - 7);
                while (str.size() && !isdigit(str[0]))
                    str.erase(0, 1);
                while (str.size() && (str[str.size() - 1] == ' ' || !std::isgraph(static_cast<unsigned char>(str[str.size() - 1]))))
                    str.erase(str.size() - 1, 1);
                fProtVersionString = str;
                if (verbose > 4)
                    emit toConsole("caught PROTVER string: '" + QString::fromStdString(fProtVersionString) + "'\n");
                float nr = QtSerialUblox::getProtVersion();
                if (verbose > 4)
                    emit toConsole("ver: " + QString::number(nr) + "\n");
            }
            if (verbose > 3) {
                emit toConsole(QString::fromStdString(s) + "\n");
            }
        }
        i = msg.find((char)0x00, i + 1);
    }
    emit gpsVersion(QString::fromStdString(swString), QString::fromStdString(hwString), QString::fromStdString(fProtVersionString));
}

void QtSerialUblox::UBXMonTx(const std::string& msg)
{
    // parse all fields
    // nr bytes pending
    uint16_t pending[s_nr_targets];
    uint8_t usage[s_nr_targets];
    uint8_t peakUsage[s_nr_targets];

    for (std::size_t target = 0; target < s_nr_targets; target++) {
        pending[target] = get<uint16_t>(msg.begin() + 2 * target);
        usage[target] = get<uint8_t>(msg.begin() + target + 12);
        peakUsage[target] = get<uint8_t>(msg.begin() + target + 18);
    }

    auto tUsage = get<uint8_t>(msg.begin() + 24);
    auto tPeakUsage = get<uint8_t>(msg.begin() + 25);

    // meaning of columns:
    // 01 21 - signature of NAV-TIMEUTC message
    // second in current week, year, month, day, hour, minute, seconds(+fraction)
    // accuracy (ns)
    emit UBXReceivedTxBuf(tUsage, tPeakUsage);
    txBufUsage = tUsage;
    txBufPeakUsage = tPeakUsage;

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << std::setfill(' ') << std::setw(3);
        tempStream << "*** UBX-MON-TXBUF message:" << '\n';
        tempStream << " global TX buf usage      : " << (int)tUsage << " %" << '\n';
        tempStream << " global TX buf peak usage : " << (int)tPeakUsage << " %" << '\n';
        tempStream << " TX buf usage for target      : ";
        for (std::size_t i = 0; i < s_nr_targets; i++) {
            tempStream << "    (" << i << ") " << std::setw(3) << (int)usage[i];
        }
        tempStream << '\n';
        tempStream << " TX buf peak usage for target : ";
        for (std::size_t i = 0; i < s_nr_targets; i++) {
            tempStream << "    (" << i << ") " << std::setw(3) << (int)peakUsage[i];
        }
        tempStream << '\n';
        tempStream << " TX bytes pending for target  : ";
        for (std::size_t i = 0; i < s_nr_targets; i++) {
            tempStream << "    (" << i << ") " << std::setw(3) << pending[i];
        }
        tempStream << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXMonRx(const std::string& msg)
{
    // parse all fields
    // nr bytes pending
    uint16_t pending[s_nr_targets];
    uint8_t usage[s_nr_targets];
    uint8_t peakUsage[s_nr_targets];
    uint8_t tUsage { 0 };
    uint8_t tPeakUsage { 0 };

    for (std::size_t target = 0; target < s_nr_targets; target++) {
        pending[target] = get<uint16_t>(msg.begin() + 2 * target);
        usage[target] = get<uint8_t>(msg.begin() + target + 12);
        peakUsage[target] = get<uint8_t>(msg.begin() + target + 18);
        tUsage += usage[target];
        tPeakUsage += peakUsage[target];
    }

    emit UBXReceivedRxBuf(tUsage, tPeakUsage);

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << std::setfill(' ') << std::setw(3);
        tempStream << "*** UBX-MON-RXBUF message:" << '\n';
        tempStream << " global RX buf usage      : " << (int)tUsage << " %" << '\n';
        tempStream << " global RX buf peak usage : " << (int)tPeakUsage << " %" << '\n';
        tempStream << " RX buf usage for target      : ";
        for (std::size_t i = 0; i < s_nr_targets; i++) {
            tempStream << "    (" << i << ") " << std::setw(3) << (int)usage[i];
        }
        tempStream << '\n';
        tempStream << " RX buf peak usage for target : ";
        for (std::size_t i = 0; i < s_nr_targets; i++) {
            tempStream << "    (" << i << ") " << std::setw(3) << (int)peakUsage[i];
        }
        tempStream << '\n';
        tempStream << " RX bytes pending for target  : ";
        for (std::size_t i = 0; i < s_nr_targets; i++) {
            tempStream << "    (" << i << ") " << std::setw(3) << pending[i];
        }
        tempStream << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXCfgNavX5(const std::string& msg)
{
    // parse all fields
    auto version { get<uint8_t>(msg.begin()) };
    /* auto mask1 { get<uint16_t>(msg.begin() + 1) }; */
    auto minSVs { get<uint8_t>(msg.begin() + 10) };
    auto maxSVs { get<uint8_t>(msg.begin() + 11) };
    auto minCNO { get<uint8_t>(msg.begin() + 12) };
    auto iniFix3D { get<uint8_t>(msg.begin() + 14) };
    auto wknRollover { get<uint16_t>(msg.begin() + 18) };
    auto aopCfg { get<uint8_t>(msg.begin() + 27) };
    auto aopOrbMaxErr { get<uint16_t>(msg.begin() + 30) };

    if (verbose > 2) {
        std::stringstream tempStream;
        tempStream << "*** UBX-MON-NAVX5 message:" << '\n';
        tempStream << " msg version         : " << (int)version << '\n';
        tempStream << " min nr of SVs       : " << (int)minSVs << '\n';
        tempStream << " max nr of SVs       : " << (int)maxSVs << '\n';
        tempStream << " min CNR for nav     : " << (int)minCNO << '\n';
        tempStream << " initial 3D fix      : " << (int)iniFix3D << '\n';
        tempStream << " GPS week rollover   : " << (int)wknRollover << '\n';
        tempStream << " AOP auton config    : " << (int)aopCfg << '\n';
        tempStream << " max AOP orbit error : " << (int)aopOrbMaxErr << " m" << '\n';
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXCfgAnt(const std::string& msg)
{
    // parse all fields
    auto flags { get<uint16_t>(msg.begin()) };
    auto pins { get<uint16_t>(msg.begin() + 2) };

    if (verbose > 2) {
        std::stringstream tempStream;
        tempStream << "*** UBX-CFG-ANT message:" << '\n';
        tempStream << " flags                     : 0x" << std::hex << (int)flags << std::dec << '\n';
        tempStream << " ant supply control signal : " << std::string((flags & 0x01) ? "on" : "off") << '\n';
        tempStream << " short detection           : " << std::string((flags & 0x02) ? "on" : "off") << '\n';
        tempStream << " open detection            : " << std::string((flags & 0x04) ? "on" : "off") << '\n';
        tempStream << " pwr down on short         : " << std::string((flags & 0x08) ? "on" : "off") << '\n';
        tempStream << " auto recovery from short  : " << std::string((flags & 0x10) ? "on" : "off") << '\n';
        tempStream << " supply switch pin         : " << (int)(pins & 0x1f) << '\n';
        tempStream << " short detection pin       : " << (int)((pins >> 5) & 0x1f) << '\n';
        tempStream << " open detection pin        : " << (int)((pins >> 10) & 0x1f) << '\n';

        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXCfgTP5(const std::string& msg)
{
    UbxTimePulseStruct tp {};
    // parse all fields
    tp.tpIndex = get<uint8_t>(msg.begin());
    tp.version = get<uint8_t>(msg.begin() + 1);
    tp.antCableDelay = get<int16_t>(msg.begin() + 4);
    tp.rfGroupDelay = get<int16_t>(msg.begin() + 6);
    tp.freqPeriod = get<uint32_t>(msg.begin() + 8);
    tp.freqPeriodLock = get<uint32_t>(msg.begin() + 12);
    tp.pulseLenRatio = get<uint32_t>(msg.begin() + 16);
    tp.pulseLenRatioLock = get<uint32_t>(msg.begin() + 20);
    tp.userConfigDelay = get<int32_t>(msg.begin() + 24);
    tp.flags = get<uint32_t>(msg.begin() + 28);

    bool isFreq { tp.flags & 0x08 };
    bool isLength { tp.flags & 0x10 };

    if (verbose > 2) {
        std::stringstream tempStream;
        tempStream << "*** UBX-CFG-TP5 message:" << '\n';
        tempStream << " message version           : " << std::dec << (int)tp.version << '\n';
        tempStream << " time pulse index          : " << std::dec << (int)tp.tpIndex << '\n';
        tempStream << " ant cable delay           : " << std::dec << (int)tp.antCableDelay << " ns" << '\n';
        tempStream << " rf group delay            : " << std::dec << (int)tp.rfGroupDelay << " ns" << '\n';
        tempStream << " user config delay         : " << std::dec << (int)tp.userConfigDelay << " ns" << '\n';
        if (isFreq) {
            tempStream << " pulse frequency           : " << std::dec << (int)tp.freqPeriod << " Hz" << '\n';
            tempStream << " locked pulse frequency    : " << std::dec << (int)tp.freqPeriodLock << " Hz" << '\n';
        } else {
            tempStream << " pulse period              : " << std::dec << (int)tp.freqPeriod << " us" << '\n';
            tempStream << " locked pulse period       : " << std::dec << (int)tp.freqPeriodLock << " us" << '\n';
        }
        if (isLength) {
            tempStream << " pulse length              : " << std::dec << (int)tp.pulseLenRatio << " us" << '\n';
            tempStream << " locked pulse length       : " << std::dec << (int)tp.pulseLenRatioLock << " us" << '\n';
        } else {
            tempStream << " pulse duty cycle          : " << std::dec << (double)tp.pulseLenRatio / ((uint64_t)1 << 32) << '\n';
            tempStream << " locked pulse duty cycle   : " << std::dec << (double)tp.pulseLenRatioLock / ((uint64_t)1 << 32) << '\n';
        }
        tempStream << " flags                     : 0x" << std::hex << (int)tp.flags << std::dec << '\n';
        tempStream << " tp active                 : " << std::string((tp.flags & 0x01) ? "yes" : "no") << '\n';

        tempStream << " lockGpsFreq               : " << std::string((tp.flags & 0x02) ? "on" : "off") << '\n';
        tempStream << " lockedOtherSet            : " << std::string((tp.flags & 0x04) ? "on" : "off") << '\n';
        tempStream << " isFreq                    : " << std::string((tp.flags & 0x08) ? "on" : "off") << '\n';
        tempStream << " isLength                  : " << std::string((tp.flags & 0x10) ? "on" : "off") << '\n';
        tempStream << " alignToTow                : " << std::string((tp.flags & 0x20) ? "on" : "off") << '\n';
        tempStream << " polarity                  : " << std::string((tp.flags & 0x40) ? "rising" : "falling") << '\n';
        tempStream << " time grid                 : ";
        if (getProtVersion() < 16)
            tempStream << std::string((tp.flags & 0x80) ? "GPS" : "UTC") << '\n';
        else {
            int timeGrid = (tp.flags & UbxTimePulseStruct::GRID_UTC_GPS) >> 7;
            switch (timeGrid) {
            case 0:
                tempStream << "UTC" << '\n';
                break;
            case 1:
                tempStream << "GPS" << '\n';
                break;
            case 2:
                tempStream << "Glonass" << '\n';
                break;
            case 3:
                tempStream << "BeiDou" << '\n';
                break;
            case 4:
                tempStream << "Galileo" << '\n';
                break;
            default:
                tempStream << "unknown" << '\n';
            }
        }

        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    emit UBXReceivedTP5(tp);
}

void QtSerialUblox::UBXSetCfgTP5(const UbxTimePulseStruct& tp)
{
    std::array<unsigned char, 32> buf { 0 };
    put<std::uint8_t>(buf.begin(), tp.tpIndex);
    put<std::uint8_t>(buf.begin() + 1, 0);
    put<std::uint16_t>(buf.begin() + 4, tp.antCableDelay);
    put<std::uint16_t>(buf.begin() + 6, tp.rfGroupDelay);
    put<std::uint32_t>(buf.begin() + 8, tp.freqPeriod);
    put<std::uint32_t>(buf.begin() + 12, tp.freqPeriodLock);
    put<std::uint32_t>(buf.begin() + 16, tp.pulseLenRatio);
    put<std::uint32_t>(buf.begin() + 20, tp.pulseLenRatioLock);
    put<std::uint32_t>(buf.begin() + 24, tp.userConfigDelay);
    put<std::uint32_t>(buf.begin() + 28, tp.flags);
    enqueueMsg(UBX_MSG::CFG_TP5, toStdString(buf.data(), buf.size()));
}

void QtSerialUblox::UBXNavDOP(const std::string& msg)
{
    // UBX-NAV-DOP: dilution of precision values
    UbxDopStruct d {};

    // parse all fields
    auto iTOW { get<uint32_t>(msg.begin()) };
    // geometric DOP
    d.gDOP = get<uint16_t>(msg.begin() + 4);
    // position DOP
    d.pDOP = get<uint16_t>(msg.begin() + 6);
    // time DOP
    d.tDOP = get<uint16_t>(msg.begin() + 8);
    // vertical DOP
    d.vDOP = get<uint16_t>(msg.begin() + 10);
    // horizontal DOP
    d.hDOP = get<uint16_t>(msg.begin() + 12);
    // northing DOP
    d.nDOP = get<uint16_t>(msg.begin() + 14);
    // easting DOP
    d.eDOP = get<uint16_t>(msg.begin() + 16);

    emit UBXReceivedDops(d);

    if (verbose > 3) {
        std::stringstream tempStream;
        tempStream << "*** UBX NAV-DOP message:" << '\n';
        tempStream << " iTOW             : " << std::dec << iTOW / 1000 << " s" << '\n';
        tempStream << " geometric DOP    : " << std::dec << d.gDOP / 100. << '\n';
        tempStream << " position DOP     : " << std::dec << d.pDOP / 100. << '\n';
        tempStream << " time DOP         : " << std::dec << d.tDOP / 100. << '\n';
        tempStream << " vertical DOP     : " << std::dec << d.vDOP / 100. << '\n';
        tempStream << " horizontal DOP   : " << std::dec << d.hDOP / 100. << '\n';
        tempStream << " northing DOP     : " << std::dec << d.nDOP / 100. << '\n';
        tempStream << " easting DOP      : " << std::dec << d.eDOP / 100. << '\n';
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}
