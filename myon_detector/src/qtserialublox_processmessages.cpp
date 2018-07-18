#include <sstream>
#include <QThread>
#include <qtserialublox.h>
#include <unixtime_from_gps.h>

using namespace std;

const uint8_t usedPort = 1; // this is the uart port. (0 = i2c; 1 = uart; 2 = usb; 3 = isp;)
                            // see u-blox8-M8_Receiver... pdf documentation site 170
// all about processing different ubx-messages:
void QtSerialUblox::processMessage(const UbxMessage& msg)
{
    uint8_t classID = (msg.msgID & 0xff00) >> 8;
    uint8_t messageID = msg.msgID & 0xff;
    std::vector<GnssSatellite> sats;
    std::stringstream tempStream;
    //std::string temp;
    uint16_t ackedMsgID;
    switch (classID) {
    case 0x05: // UBX-ACK
        if(msg.data.size()<2){
            emit toConsole("received UBX-ACK message but data is corrupted\n");
            break;
        }
        if (verbose > 3){
            tempStream << "received UBX-ACK message about msgID: 0x"
                       <<std::setfill('0') << std::setw(2) << std::hex << (int)msg.data[0] << " 0x"
                       <<std::setfill('0') << std::setw(2) << std::hex << (int)msg.data[1] << "\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        ackedMsgID = (uint16_t)(msg.data[0]) << 8 | msg.data[1];
        if (!msgWaitingForAck){
            if (verbose > 1){
                emit toConsole("received Ack message but no message is waiting for Ack\n");
            }
            break;
        }
        if (ackedMsgID!=msgWaitingForAck->msgID){
            if (verbose > 1){
                emit toConsole("received unexpected Ack message\n");
            }
            break;
        }
        switch (messageID){
        case 0x00:
            emit UBXReceivedAckNak(msgWaitingForAck->msgID,
                                      (uint16_t)(msgWaitingForAck->data[0])<<8
                                     |msgWaitingForAck->data[1]);
            sendQueuedMsg(true); // tries to send the message again
            break;
        default:
            ackTimer->stop();
            delete msgWaitingForAck;
            msgWaitingForAck = 0;
            sendQueuedMsg();
            break;
        }
        break;
    case 0x01: // UBX-NAV
        switch (messageID) {
        case 0x20:
            if (verbose > 3) {
                tempStream << "received UBX-NAV-TIMEGPS message (0x" << std::hex <<std::setfill('0') << std::setw(2)
                           << (int)classID << " 0x" << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            UBXNavTimeGPS(msg.data);
            break;
        case 0x21:
            if (verbose > 3) {
                tempStream << "received UBX-NAV-TIMEUTC message (0x" << std::hex << std::setw(2)
                           << (int)classID << " 0x" << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            UBXNavTimeUTC(msg.data);
            break;
        case 0x22:
            if (verbose > 3) {
                tempStream << "received unhandled UBX-NAV-CLOCK message (0x" << std::hex <<std::setfill('0') << std::setw(2)
                           << (int)classID << " 0x" << std::hex << (int)messageID << ")\n" ;
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            UBXNavClock(msg.data);
            break;
        case 0x35:
            sats = UBXNavSat(msg.data, true);
            if (verbose > 3) {
                tempStream << "received unhandled UBX-NAV-SAT message (0x" << std::hex << std::setfill('0') << std::setw(2) << (int)classID
                           << " 0x" << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            //mutex.lock();
            emit gpsPropertyUpdatedGnss(sats, satList.updateAge());
            satList = sats;
            //mutex.unlock();
            //break;
//				tempStream<<"Satellite List: "<<sats.size()<<" sats received"<<endl;
// 				GnssSatellite::PrintHeader(true);
// 				for (int i=0; i<sats.size(); i++) sats[i].Print(i, false);
            break;
        default:
            if (verbose > 3) {
                tempStream << "received unhandled UBX-NAV message (0x" << std::hex <<std::setfill('0') << std::setw(2) << (int)classID
                           << " 0x" << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
        }
        break;
    case 0x02: // UBX-RXM
        if (verbose > 3){
            tempStream << "received unhandled UBX-RXM message\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        break;
    case 0x0b: // UBX-AID
        if (verbose > 3){
            tempStream << "received unhandled UBX-AID message\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        break;
    case 0x06: // UBX-CFG
        switch (messageID) {
        case 0x01: // UBX-CFG-MSG   (message configuration)
            emit UBXreceivedMsgRateCfg(
                        (((uint16_t)msg.data[0])<<8)|((uint16_t)msg.data[1]),
                        (uint8_t)(msg.data[2+usedPort])
                    );
            // 2: port 0 (i2c); 3: port 1 (uart); 4: port 2 (usb); 5: port 3 (isp)
        default:
            if (verbose > 1) {
                tempStream << "received UBX-CFG message:";
                for (std::string::size_type i = 0; i<msg.data.size(); i++){
                    tempStream << " 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)msg.data[i];
                }
                tempStream << "\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
        }
        break;
    case 0x10: // UBX-ESF
        if (verbose > 3){
            tempStream << "received unhandled UBX-ESF message\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        break;
    case 0x28: // UBX-HNR
        if (verbose > 3){
            tempStream << "received unhandled UBX-HNR message\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        break;
    case 0x04: // UBX-INF
        switch (messageID) {
        default:
            if (verbose > 3) {
                tempStream << "received unhandled UBX-INF message (0x" << std::hex <<std::setfill('0') << std::setw(2) << (int)classID
                           << " 0x" << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
        }
        break;
    case 0x21: // UBX-LOG
        if (verbose > 3){
            tempStream << "received unhandled UBX-LOG message\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        break;
    case 0x13: // UBX-MGA
        if (verbose > 3){
            tempStream << "received unhandled UBX-MGA message\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        break;
    case 0x0a: // UBX-MON
        switch (messageID) {
        case 0x08:
            if (verbose > 3) {
                tempStream << "received UBX-MON-TXBUF message (0x" << std::hex <<std::setfill('0') << std::setw(2) << (int)classID
                           << " 0x" << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            UBXMonTx(msg.data);
            break;
        case 0x09:
            if (verbose > 3) {
                tempStream << "received UBX-MON-HW message (0x" << std::hex <<std::setfill('0') << std::setw(2) << (int)classID
                           << " 0x" << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            UBXMonHW(msg.data);
            break;
        default:
            if (verbose > 3) {
                tempStream << "received unhandled UBX-MON message (0x" << std::hex <<std::setfill('0') << std::setw(2) << (int)classID
                           << " 0x" << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
        }
        break;
    case 0x27: // UBX-SEC
        if (verbose > 3){
            tempStream << "received unhandled UBX-SEC message\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        break;
    case 0x0d: // UBX-TIM
        switch (messageID) {
        case 0x01: // UBX-TIM-TP
            if (verbose > 3) {
                tempStream << "received UBX-TIM-TP message (0x" << std::hex <<std::setfill('0') << std::setw(2) << (int)classID << " 0x"
                           << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            UBXTimTP(msg.data);
            break;
        case 0x03: // UBX-TIM-TM2
            if (verbose > 3) {
                tempStream << "received UBX-TIM-TM2 message (0x" << std::hex <<std::setfill('0') << std::setw(2) << (int)classID << " 0x"
                           << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            UBXTimTM2(msg.data);
            break;
        default:
            if (verbose > 3) {
                tempStream << "received unhandled UBX-TIM message (0x" << std::hex <<std::setfill('0') << std::setw(2) << (int)classID
                           << " 0x" << std::hex << (int)messageID << ")\n";
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
        }
        break;
    case 0x09: // UBX-UPD
        if (verbose > 3){
            tempStream << "received unhandled UBX-UPD message\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        break;
    default:
        if (verbose > 3){
            tempStream << "received unknown UBX message (0x" << std::hex <<std::setfill('0') << std::setw(2) << (int)classID
                       << " 0x" << std::hex << (int)messageID << ")\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        break;
    }
}

bool QtSerialUblox::UBXNavClock(uint32_t& itow, int32_t& bias, int32_t& drift,
                        uint32_t& tAccuracy, uint32_t& fAccuracy)
{
    // why tAccuracy?
    tAccuracy=0;
    std::string answer;
    // UBX-NAV-CLOCK: clock solution
    //bool ok = pollUBX(MSG_NAV_CLOCK, answer, MSGTIMEOUT);
    //if (!ok) return ok;
    //return true;
    // parse all fields
    // GPS time of week
    uint32_t iTOW = (int)answer[0];
    iTOW += ((int)answer[1]) << 8;
    iTOW += ((int)answer[2]) << 16;
    iTOW += ((int)answer[3]) << 24;
    itow = iTOW / 1000;
    // clock bias
    if (verbose > 3) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "clkB[0]=" << std::hex << (int)answer[4] << endl;
        tempStream << "clkB[1]=" << std::hex << (int)answer[5] << endl;
        tempStream << "clkB[2]=" << std::hex << (int)answer[6] << endl;
        tempStream << "clkB[3]=" << std::hex << (int)answer[7] << endl;
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    int32_t clkB = (int)answer[4];
    clkB += ((int)answer[5]) << 8;
    clkB += ((int)answer[6]) << 16;
    clkB += ((int)answer[7]) << 24;
    bias = clkB;
    // clock drift
    int32_t clkD = (int)answer[8];
    clkD += ((int)answer[9]) << 8;
    clkD += ((int)answer[10]) << 16;
    clkD += ((int)answer[11]) << 24;
    drift = clkD;
    // time accuracy estimate
    uint32_t tAcc = (int)answer[12];
    tAcc += ((int)answer[13]) << 8;
    tAcc += ((int)answer[14]) << 16;
    tAcc += ((int)answer[15]) << 24;
    //  tAccuracy=tAcc;
      // freq accuracy estimate
    uint32_t fAcc = (int)answer[16];
    fAcc += ((int)answer[17]) << 8;
    fAcc += ((int)answer[18]) << 16;
    fAcc += ((int)answer[19]) << 24;
    fAccuracy = fAcc;
    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-CLOCK message:" << endl;
        tempStream << " iTOW          : " << dec << iTOW / 1000 << " s" << endl;
        tempStream << " clock bias    : " << dec << clkB << " ns" << endl;
        tempStream << " clock drift   : " << dec << clkD << " ns/s" << endl;
        tempStream << " time accuracy : " << dec << tAcc << " ns" << endl;
        tempStream << " freq accuracy : " << dec << fAcc << " ps/s\n" << endl;
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    return true;
}

bool QtSerialUblox::UBXTimTP(uint32_t& itow, int32_t& quantErr, uint16_t& weekNr)
{
    std::string answer;
    // UBX-TIM-TP: time pulse timedata
    //bool ok = pollUBX(MSG_TIM_TP, answer, MSGTIMEOUT);
    //if (!ok) return ok;
    // parse all fields
    // TP time of week, ms
    uint32_t towMS = (int)answer[0];
    towMS += ((int)answer[1]) << 8;
    towMS += ((int)answer[2]) << 16;
    towMS += ((int)answer[3]) << 24;
    // TP time of week, sub ms
    uint32_t towSubMS = (int)answer[4];
    towSubMS += ((int)answer[5]) << 8;
    towSubMS += ((int)answer[6]) << 16;
    towSubMS += ((int)answer[7]) << 24;
    itow = towMS / 1000;
    // quantization error
    int32_t qErr = (int)answer[8];
    qErr += ((int)answer[9]) << 8;
    qErr += ((int)answer[10]) << 16;
    qErr += ((int)answer[11]) << 24;
    quantErr = qErr;
    // week number
    uint16_t week = (int)answer[12];
    week += ((int)answer[13]) << 8;
    weekNr = week;
    // flags
    uint8_t flags = answer[14];
    // ref info
    uint8_t refInfo = answer[15];

    double sr = towMS / 1000.;
    sr = sr - towMS / 1000;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-TIM-TP message:" << endl;
        tempStream << " tow s            : " << dec << towMS / 1000. << " s" << endl;
        tempStream << " tow sub s        : " << dec << towSubMS << " = " << (long int)(sr*1e9 + towSubMS + 0.5) << " ns" << endl;
        tempStream << " quantization err : " << dec << qErr << " ps" << endl;
        tempStream << " week nr          : " << dec << week << endl;
        tempStream << " flags            : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << " refInfo          : ";
        for (int i = 7; i >= 0; i--) if (refInfo & 1 << i) tempStream << i; else tempStream << "-";
        //tempStream >> temp;
        tempStream << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    return true;
}

bool QtSerialUblox::UBXTimTP()
{
    std::string answer;
    // UBX-TIM-TP: time pulse timedata
    //bool ok = pollUBX(MSG_TIM_TP, answer, MSGTIMEOUT);
    //if (!ok) return ok;
    UBXTimTP(answer);
    return true;
}

bool QtSerialUblox::UBXTimTP(const std::string& msg)
{
    // parse all fields
    // TP time of week, ms
    uint32_t towMS = (int)msg[0];
    towMS += ((int)msg[1]) << 8;
    towMS += ((int)msg[2]) << 16;
    towMS += ((int)msg[3]) << 24;
    // TP time of week, sub ms
    uint32_t towSubMS = (int)msg[4];
    towSubMS += ((int)msg[5]) << 8;
    towSubMS += ((int)msg[6]) << 16;
    towSubMS += ((int)msg[7]) << 24;
    //int itow = towMS / 1000;
    // quantization error
    int32_t qErr = (int)msg[8];
    qErr += ((int)msg[9]) << 8;
    qErr += ((int)msg[10]) << 16;
    qErr += ((int)msg[11]) << 24;
    //int quantErr = qErr;
    //mutex.lock();
    emit gpsPropertyUpdatedInt32(qErr,TPQuantErr.updateAge(),'e');
    TPQuantErr = qErr;
    //mutex.unlock();
    // week number
    uint16_t week = (int)msg[12];
    week += ((int)msg[13]) << 8;
    //int weekNr = week;
    // flags
    uint8_t flags = msg[14];
    // ref info
    uint8_t refInfo = msg[15];

    double sr = towMS / 1000.;
    sr = sr - towMS / 1000;

    //   cout<<"0d 01 "<<dec<<weekNr<<" "<<towMS/1000<<" "<<(long int)(sr*1e9+towSubMS+0.5)<<" "<<qErr<<flush<<endl;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-TIM-TP message:" << endl;
        tempStream << " tow s            : " << dec << towMS / 1000. << " s" << endl;
        tempStream << " tow sub s        : " << dec << towSubMS << " = " << (long int)(sr*1e9 + towSubMS + 0.5) << " ns" << endl;
        tempStream << " quantization err : " << dec << qErr << " ps" << endl;
        tempStream << " week nr          : " << dec << week << endl;
        tempStream << " *flags            : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "  time base     : " << string(((flags & 1) ? "UTC" : "GNSS")) << endl;
        tempStream << "  UTC available : " << string((flags & 2) ? "yes" : "no") << endl;
        tempStream << "  (T)RAIM info  : " << (int)((flags & 0x0c) >> 2) << endl;
        tempStream << " *refInfo          : ";
        for (int i = 7; i >= 0; i--) if (refInfo & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        string gnssRef;
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
        tempStream << "  GNSS reference : " << gnssRef << endl;
        string utcStd;
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
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    return true;
}

bool QtSerialUblox::UBXTimTM2(const std::string& msg)
{
    // parse all fields
    // channel
    uint8_t ch = msg[0];
    // flags
    uint8_t flags = msg[1];
    // rising edge counter
    uint16_t count = (int)msg[2]; // 16 bit counter
    count += ((int)msg[3]) << 8;
    // week number of last rising edge
    uint16_t wnR = (int)msg[4];
    wnR += ((int)msg[5]) << 8;
    // week number of last falling edge
    uint16_t wnF = (int)msg[6];
    wnF += ((int)msg[7]) << 8;
    // time of week of rising edge, ms
    uint32_t towMsR = (int)msg[8];
    towMsR += ((int)msg[9]) << 8;
    towMsR += ((int)msg[10]) << 16;
    towMsR += ((int)msg[11]) << 24;
    // time of week of rising edge, sub ms
    uint32_t towSubMsR = (int)msg[12];
    towSubMsR += ((int)msg[13]) << 8;
    towSubMsR += ((int)msg[14]) << 16;
    towSubMsR += ((int)msg[15]) << 24;
    // time of week of falling edge, ms
    uint32_t towMsF = (int)msg[16];
    towMsF += ((int)msg[17]) << 8;
    towMsF += ((int)msg[18]) << 16;
    towMsF += ((int)msg[19]) << 24;
    // time of week of falling edge, sub ms
    uint32_t towSubMsF = (int)msg[20];
    towSubMsF += ((int)msg[21]) << 8;
    towSubMsF += ((int)msg[22]) << 16;
    towSubMsF += ((int)msg[23]) << 24;
    // accuracy estimate
    uint32_t accEst = (int)msg[24];
    accEst += ((int)msg[25]) << 8;
    accEst += ((int)msg[26]) << 16;
    accEst += ((int)msg[27]) << 24;

    //mutex.lock();
    emit gpsPropertyUpdatedUint32(accEst, timeAccuracy.updateAge(), 'a');
    timeAccuracy = accEst;
    timeAccuracy.lastUpdate = std::chrono::system_clock::now();
    //mutex.unlock();

    double sr = towMsR / 1000.;
    sr = sr - towMsR / 1000;
    //  sr*=1000.;
    double sf = towMsF / 1000.;
    sf = sf - towMsF / 1000;
    //  sf*=1000.;
    //  cout<<"sr="<<sr<<" sf="<<sf<<endl;

      // meaning of columns:
      // 0d 03 - signature of TIM-TM2 message
      // ch, week nr, second in current week (rising), ns of timestamp in current second (rising),
      // second in current week (falling), ns of timestamp in current second (falling),
      // accuracy (ns), rising edge counter, rising/falling edge (1/0), time valid (GNSS fix)
    //   cout<<"0d 03 "<<dec<<(int)ch<<" "<<wnR<<" "<<towMsR/1000<<" "<<(long int)(sr*1e9+towSubMsR)<<" "<<towMsF/1000<<" "<<(long int)(sf*1e9+towSubMsF)<<" "<<accEst<<" "<<count<<" "<<string((flags&0x80)?"1":"0")<<" "<<string((flags&0x40)?"1":"0")<<flush;
    //   cout<<endl;
    //  cout<<flush;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-TimTM2 message:" << endl;
        tempStream << " channel         : " << dec << (int)ch << endl;
        tempStream << " rising edge ctr : " << dec << count << endl;
        tempStream << " * last rising edge:" << endl;
        tempStream << "    week nr        : " << dec << wnR << endl;
        tempStream << "    tow s          : " << dec << towMsR / 1000. << " s" << endl;
        tempStream << "    tow sub s     : " << dec << towSubMsR << " = " << (long int)(sr*1e9 + towSubMsR) << " ns" << endl;
        tempStream << " * last falling edge:" << endl;
        tempStream << "    week nr        : " << dec << wnF << endl;
        tempStream << "    tow s          : " << dec << towMsF / 1000. << " s" << endl;
        tempStream << "    tow sub s      : " << dec << towSubMsF << " = " << (long int)(sf*1e9 + towSubMsF) << " ns" << endl;
        tempStream << " accuracy est      : " << dec << accEst << " ns" << endl;
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "   mode                 : " << string((flags & 1) ? "single" : "running") << endl;
        tempStream << "   run                  : " << string((flags & 2) ? "armed" : "stopped") << endl;
        tempStream << "   new rising edge      : " << string((flags & 0x80) ? "yes" : "no") << endl;
        tempStream << "   new falling edge     : " << string((flags & 0x04) ? "yes" : "no") << endl;
        tempStream << "   UTC available        : " << string((flags & 0x20) ? "yes" : "no") << endl;
        tempStream << "   time valid (GNSS fix): " << string((flags & 0x40) ? "yes" : "no") << endl;
        string timeBase;
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
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }else if(verbose > 0){
        // output is: rising falling accEst valid timeBase utcAvailable
        std::stringstream tempStream;
        if (((flags & 0x80)>>7)==1){
            // if new rising edge
            tempStream << unixtime_from_gps(wnR, towMsR / 1000, (long int)(sr*1e9 + towSubMsR));
        }else{
            tempStream << "..................... ";
        }
        if (((flags & 0x4)>>2)==1){
            // if new falling edge
            tempStream << unixtime_from_gps(wnF, towMsF / 1000, (long int)(sr*1e9 + towSubMsF));
        }else{
            tempStream << "..................... ";
        }
        tempStream << accEst
                   << " " << count
                   << " " << ((flags & 0x40)>>6)
                   << " " << setfill('0') << setw(2) << ((flags & 0x18)>>3)
                   << " " << ((flags & 0x20)>>5) << endl;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }

    struct timespec ts_r = unixtime_from_gps(wnR, towMsR / 1000, (long int)(sr*1e9 + towSubMsR)/*, this->leapSeconds()*/);
    struct timespec ts_f = unixtime_from_gps(wnF, towMsF / 1000, (long int)(sf*1e9 + towSubMsF)/*, this->leapSeconds()*/);

    struct gpsTimestamp ts;
    ts.rising_time = ts_r;
    ts.falling_time = ts_f;
    ts.valid = (flags & 0x40);
    ts.channel = ch;

    //   fTimestamps.push(ts);
    ts.counter = count;
    ts.accuracy_ns = accEst;

    ts.rising = ts.falling = false;
    if (flags & 0x80) {
        // new rising edge detected
        ts.rising = true;


    } if (flags & 0x04) {
        // new falling edge detected
        ts.falling = true;

    }
    //mutex.lock();
    emit gpsPropertyUpdatedUint32(count, eventCounter.updateAge(),'c');
    eventCounter = count;
    fTimestamps.push(ts);
    //mutex.unlock();

    return true;
}

std::vector<GnssSatellite> QtSerialUblox::UBXNavSat(bool allSats)
{
    std::string answer;
    std::vector<GnssSatellite> satList;
    // UBX-NAV-SAT: satellite information
    //bool ok = pollUBX(MSG_NAV_SAT, answer, MSGTIMEOUT);
    //  bool ok=pollUBX(0x0d, 0x03, 0, answer, MSGTIMEOUT);
    //if (!ok) return satList;

    return UBXNavSat(answer, allSats);

    // parse all fields
    // GPS time of week
    uint32_t iTOW = (int)answer[0];
    iTOW += ((int)answer[1]) << 8;
    iTOW += ((int)answer[2]) << 16;
    iTOW += ((int)answer[3]) << 24;
    // version
    uint8_t version = answer[4];
    uint8_t numSvs = answer[5];

    int N = (answer.size() - 8) / 12;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-SAT message:" << endl;
        tempStream << " iTOW          : " << dec << iTOW / 1000 << " s" << endl;
        tempStream << " version       : " << dec << (int)version << endl;
        tempStream << " Nr of sats    : " << (int)numSvs << "  (nr of sections=" << N << ")\n";
        tempStream << "   Sat Data :\n";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    int goodSats = 0;
    for (int i = 0; i < N; i++) {
        GnssSatellite sat(answer.substr(8 + 12 * i, 12));
        if (sat.getCnr() > 0) goodSats++;
        satList.push_back(sat);
    }
    if (!allSats) {
        sort(satList.begin(), satList.end(), GnssSatellite::sortByCnr);
        while (satList.back().getCnr() == 0 && satList.size() > 0) satList.pop_back();
    }

    if (verbose>0) {
        std::string temp;
        GnssSatellite::PrintHeader(true);
        for (vector<GnssSatellite>::iterator it = satList.begin(); it != satList.end(); it++) {
            it->Print(distance(satList.begin(), it), false);
        }
        std::stringstream tempStream;
        tempStream << "   --------------------------------------------------------------------\n";
        if (verbose > 1){
            tempStream << " Nr of avail sats : \n" << goodSats;
        }
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    return satList;
}

std::vector<GnssSatellite> QtSerialUblox::UBXNavSat(const std::string& msg, bool allSats)
{
    std::vector<GnssSatellite> satList;
    // UBX-NAV-SAT: satellite information
    // parse all fields
    // GPS time of week
    uint32_t iTOW = (int)msg[0];
    iTOW += ((int)msg[1]) << 8;
    iTOW += ((int)msg[2]) << 16;
    iTOW += ((int)msg[3]) << 24;
    // version
    uint8_t version = msg[4];
    uint8_t numSvs = msg[5];

    int N = (msg.size() - 8) / 12;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << setfill(' ') << setw(3);
        tempStream << "*** UBX-NAV-SAT message:" << endl;
        tempStream << " iTOW          : " << dec << iTOW / 1000 << " s" << endl;
        tempStream << " version       : " << dec << (int)version << endl;
        tempStream << " Nr of sats    : " << (int)numSvs << "  (nr of sections=" << N << ")\n";
        tempStream << "   Sat Data :\n";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    uint8_t goodSats = 0;
    for (int i = 0; i < N; i++) {
        GnssSatellite sat(msg.substr(8 + 12 * i, 12));
        if (sat.getCnr() > 0) goodSats++;
        satList.push_back(sat);
    }
    if (!allSats) {
        sort(satList.begin(), satList.end(), GnssSatellite::sortByCnr);
        while (satList.back().getCnr() == 0 && satList.size() > 0) satList.pop_back();
    }

    //  nrSats = satList.size();
    //mutex.lock();
    emit gpsPropertyUpdatedUint8(goodSats,nrSats.updateAge(),'s');
    nrSats = goodSats;
    //mutex.unlock();

    if (verbose) {
        std::string temp;
        GnssSatellite::PrintHeader(true);
        for (vector<GnssSatellite>::iterator it = satList.begin(); it != satList.end(); it++) {
            it->Print(distance(satList.begin(), it), false);
        }
        std::stringstream tempStream;
        tempStream << "   --------------------------------------------------------------------\n";
        if (verbose > 1){
            tempStream << " Nr of avail sats : " << goodSats << "\n";
        }
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    return satList;
}


bool QtSerialUblox::UBXCfgGNSS()
{
    std::string answer;
    // UBX-CFG-GNSS: GNSS configuration
    //bool ok = pollUBX(MSG_CFG_GNSS, answer, MSGTIMEOUT);
    //if (!ok) return false;
    // parse all fields
    // version
    uint8_t version = answer[0];
    uint8_t numTrkChHw = answer[1];
    uint8_t numTrkChUse = answer[2];
    uint8_t numConfigBlocks = answer[3];

    int N = (answer.size() - 4) / 8;

    //  if (verbose>1)
    {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX CFG-GNSS message:" << endl;
        tempStream << " version                    : " << dec << (int)version << endl;
        tempStream << " nr of hw tracking channels : " << dec << (int)numTrkChHw << endl;
        tempStream << " nr of channels in use      : " << dec << (int)numTrkChUse << endl;
        tempStream << " Nr of config blocks        : " << (int)numConfigBlocks
                   << "  (nr of sections=" << N << ")";
        tempStream << "  Config Data :\n";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    for (int i = 0; i < N; i++) {
        uint8_t gnssID = answer[4 + 8 * i];
        uint8_t resTrkCh = answer[5 + 8 * i];
        uint8_t maxTrkCh = answer[6 + 8 * i];
        uint32_t flags = answer[8 + 8 * i];
        flags |= (int)answer[9 + 8 * i] << 8;
        flags |= (int)answer[10 + 8 * i] << 16;
        flags |= (int)answer[11 + 8 * i] << 24;
        //    if (verbose>1)
        {
            std::stringstream tempStream;
            //std::string temp;
            //mutex.lock();
            tempStream << "   " << i << ":   GNSS name : "
                       << GnssSatellite::GNSS_ID_STRING[gnssID] << endl;
            //mutex.unlock();
            tempStream << "      reserved (min) tracking channels  : "
                       << dec << (int)resTrkCh << endl;
            tempStream << "      max nr of tracking channels used : "
                       << dec << (int)maxTrkCh << endl;
            tempStream << "      flags  : " << std::hex << (int)flags << "\n";
            //tempStream >> temp;
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
    }
    return true;
}

bool QtSerialUblox::UBXCfgNav5()
{
    std::string answer;
    // UBX CFG-NAV5: satellite information
    //bool ok = pollUBX(MSG_CFG_NAV5, answer, MSGTIMEOUT);
    //if (!ok) return false;
    // parse all fields
    // version
    uint16_t mask = answer[0];
    mask |= (int)answer[1] << 8;
    uint8_t dynModel = answer[2];
    uint8_t fixMode = answer[3];
    int32_t fixedAlt = answer[4];
    fixedAlt |= (int)answer[5] << 8;
    fixedAlt |= (int)answer[6] << 16;
    fixedAlt |= (int)answer[7] << 24;
    uint32_t fixedAltVar = answer[8];
    fixedAltVar |= (int)answer[9] << 8;
    fixedAltVar |= (int)answer[10] << 16;
    fixedAltVar |= (int)answer[11] << 24;
    int8_t minElev = answer[12];


    //  if (verbose>1)
    {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX CFG-NAV5 message:" << endl;
        tempStream << " mask               : " << std::hex << (int)mask << endl;
        tempStream << " dynamic model used : " << dec << (int)dynModel << endl;
        tempStream << " fixMode            : " << dec << (int)fixMode << endl;
        tempStream << " fixed Alt          : " << (double)fixedAlt*0.01 << " m" << endl;
        tempStream << " fixed Alt Var      : " << (double)fixedAltVar*0.0001 << " m^2" << endl;
        tempStream << " min elevation      : " << dec << (int)minElev << " deg\n";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    return true;
}

/*std::vector<std::string> QtSerialUblox::UBXMonVer()
{
    std::string answer;
    std::vector<std::string> result;
    // UBX CFG-NAV5: satellite information
    bool ok = pollUBX(MSG_MON_VER, answer, MSGTIMEOUT);
    if (!ok) return result;
    // parse all fields
    // version
    cout << "*** UBX MON-VER message:" << endl;
    //  cout<<answer<<endl;
    std::string::size_type i = 0;
    while (i != std::string::npos && i < answer.size()) {
        std::string s = answer.substr(i, answer.find((char)0x00, i + 1) - i + 1);
        while (s.size() && s[0] == 0x00) {
            s.erase(0, 1);
        }
        if (s.size()) {
            result.push_back(s);
            emit toConsole(QString::fromStdString(s));
        }
        i = answer.find((char)0x00, i + 1);
    }
    return result;
}
*/

void QtSerialUblox::UBXNavClock(const std::string& msg)
{
    // parse all fields
    // GPS time of week
    uint32_t iTOW = (int)msg[0];
    iTOW += ((int)msg[1]) << 8;
    iTOW += ((int)msg[2]) << 16;
    iTOW += ((int)msg[3]) << 24;
    //uint32_t itow = iTOW / 1000;
    // clock bias
    if (verbose > 3) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "clkB[0]=" <<std::setfill('0') << std::setw(2) << std::hex << (int)msg[4] << endl;
        tempStream << "clkB[1]=" <<std::setfill('0') << std::setw(2) << std::hex << (int)msg[5] << endl;
        tempStream << "clkB[2]=" <<std::setfill('0') << std::setw(2) << std::hex << (int)msg[6] << endl;
        tempStream << "clkB[3]=" <<std::setfill('0') << std::setw(2) << std::hex << (int)msg[7];
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    int32_t clkB = (int)msg[4];
    clkB += ((int)msg[5]) << 8;
    clkB += ((int)msg[6]) << 16;
    clkB += ((int)msg[7]) << 24;
    //int32_t bias = clkB;
    // clock drift
    int32_t clkD = (int)msg[8];
    clkD += ((int)msg[9]) << 8;
    clkD += ((int)msg[10]) << 16;
    clkD += ((int)msg[11]) << 24;
    //int32_t drift = clkD;
    //mutex.lock();
    emit gpsPropertyUpdatedInt32(clkD,clkDrift.updateAge(),'d');
    emit gpsPropertyUpdatedInt32(clkB,clkBias.updateAge(),'b');
    clkDrift = clkD;
    clkBias = clkB;
    //mutex.unlock();
    // time accuracy estimate
    uint32_t tAcc = (int)msg[12];
    tAcc += ((int)msg[13]) << 8;
    tAcc += ((int)msg[14]) << 16;
    tAcc += ((int)msg[15]) << 24;
    //uint32_t tAccuracy = tAcc;
    //timeAccuracy = tAcc;
    // freq accuracy estimate
    uint32_t fAcc = (int)msg[16];
    fAcc += ((int)msg[17]) << 8;
    fAcc += ((int)msg[18]) << 16;
    fAcc += ((int)msg[19]) << 24;
    //uint32_t fAccuracy = fAcc;

    // meaning of columns:
    // 01 22 - signature of NAV-CLOCK message
    // second in current week (s), clock bias (ns), clock drift (ns/s), time accuracy (ns), freq accuracy (ps/s)
  //   cout<<"01 22 "<<dec<<iTOW/1000<<" "<<clkB<<" "<<clkD<<" "<<tAcc<<" "<<fAcc<<flush;
  //   cout<<endl;


    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-CLOCK message:" << endl;
        tempStream << " iTOW          : " << dec << iTOW / 1000 << " s" << endl;
        tempStream << " clock bias    : " << dec << clkB << " ns" << endl;
        tempStream << " clock drift   : " << dec << clkD << " ns/s" << endl;
        tempStream << " time accuracy : " << dec << tAcc << " ns" << endl;
        tempStream << " freq accuracy : " << dec << fAcc << " ps/s\n";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}


void QtSerialUblox::UBXNavTimeGPS(const std::string& msg)
{
    // parse all fields
    // GPS time of week
    uint32_t iTOW = (int)msg[0];
    iTOW += ((int)msg[1]) << 8;
    iTOW += ((int)msg[2]) << 16;
    iTOW += ((int)msg[3]) << 24;

    int32_t fTOW = (int)msg[4];
    fTOW += ((int)msg[5]) << 8;
    fTOW += ((int)msg[6]) << 16;
    fTOW += ((int)msg[7]) << 24;

    //  int32_t ftow=iTOW/1000;

    uint16_t wnR = (int)msg[8];
    wnR += ((int)msg[9]) << 8;

    int8_t leapS = (int)msg[10];
    uint8_t flags = (int)msg[11];

    // time accuracy estimate
    uint32_t tAcc = (int)msg[12];
    tAcc += ((int)msg[13]) << 8;
    tAcc += ((int)msg[14]) << 16;
    tAcc += ((int)msg[15]) << 24;
    //uint32_t tAccuracy = tAcc;


    double sr = iTOW / 1000.;
    sr = sr - iTOW / 1000;

    // meaning of columns:
    // 01 20 - signature of NAV-TIMEGPS message
    // week nr, second in current week, ns of timestamp in current second,
    // nr of leap seconds wrt UTC, accuracy (ns)
  //   cout<<"01 20 "<<dec<<wnR<<" "<<iTOW/1000<<" "<<(long int)(sr*1e9+fTOW)<<" "<<(int)leapS<<" "<<tAcc<<flush;
  //   cout<<endl;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-TIMEGPS message:" << endl;
        tempStream << " week nr       : " << dec << wnR << endl;
        tempStream << " iTOW          : " << dec << iTOW << " ms = " << iTOW / 1000 << " s" << endl;
        tempStream << " fTOW          : " << dec << fTOW << " = " << (long int)(sr*1e9 + fTOW) << " ns" << endl;
        tempStream << " leap seconds  : " << dec << (int)leapS << " s" << endl;
        tempStream << " time accuracy : " << dec << tAcc << " ns" << endl;
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "   tow valid        : " << string((flags & 1) ? "yes" : "no") << endl;
        tempStream << "   week valid       : " << string((flags & 2) ? "yes" : "no") << endl;
        tempStream << "   leap sec valid   : " << string((flags & 4) ? "yes" : "no");
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    //mutex.lock();
    emit gpsPropertyUpdatedUint32(tAcc, timeAccuracy.updateAge(), 'a');
    timeAccuracy = tAcc;
    timeAccuracy.lastUpdate = std::chrono::system_clock::now();
    if (flags & 4) { leapSeconds = leapS; }
    //mutex.unlock();

    struct timespec ts = unixtime_from_gps(wnR, iTOW / 1000, (long int)(sr*1e9 + fTOW)/*, this->leapSeconds()*/);
    std::stringstream tempStream;
    //std::string temp;
    tempStream << ts.tv_sec << '.' << ts.tv_nsec << "\n";
    //tempStream >> temp;
    if (verbose > 1) {
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXNavTimeUTC(const std::string& msg)
{
    // parse all fields
    // GPS time of week
    uint32_t iTOW = (int)msg[0];
    iTOW += ((int)msg[1]) << 8;
    iTOW += ((int)msg[2]) << 16;
    iTOW += ((int)msg[3]) << 24;

    // time accuracy estimate
    uint32_t tAcc = (int)msg[4];
    tAcc += ((int)msg[5]) << 8;
    tAcc += ((int)msg[6]) << 16;
    tAcc += ((int)msg[7]) << 24;
    //uint32_t tAccuracy = tAcc;
    //mutex.lock();
    emit gpsPropertyUpdatedUint32(tAcc, timeAccuracy.updateAge(), 'a');
    timeAccuracy = tAcc;
    timeAccuracy.lastUpdate = std::chrono::system_clock::now();
    //mutex.unlock();

    int32_t nano = (int)msg[8];
    nano += ((int)msg[9]) << 8;
    nano += ((int)msg[10]) << 16;
    nano += ((int)msg[11]) << 24;

    uint16_t year = (int)msg[12];
    year += ((int)msg[13]) << 8;

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
  //   cout<<"01 21 "<<dec<<iTOW/1000<<" "<<year<<" "<<month<<" "<<day<<" "<<hour<<" "<<min<<" "<<(double)(sec+nano*1e-9)<<" "<<tAcc<<flush;
  //   cout<<endl;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-TIMEUTC message:" << endl;
        tempStream << " iTOW           : " << dec << iTOW << " ms = " << iTOW / 1000 << " s" << endl;
        tempStream << " nano           : " << dec << nano << " ns" << endl;
        tempStream << " date y/m/d     : " << dec << (int)year << "/" << (int)month << "/" << (int)day << endl;
        tempStream << " UTC time h:m:s : " << dec << setw(2) << setfill('0') << hour << ":" << (int)min << ":" << (double)(sec + nano * 1e-9) << endl;
        tempStream << " time accuracy : " << dec << tAcc << " ns" << endl;
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "   tow valid        : " << string((flags & 1) ? "yes" : "no") << endl;
        tempStream << "   week valid       : " << string((flags & 2) ? "yes" : "no") << endl;
        tempStream << "   UTC time valid   : " << string((flags & 4) ? "yes" : "no") << endl;
        string utcStd;
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
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}

void QtSerialUblox::UBXMonHW(const std::string& msg)
{
    // parse all fields
    // noise
    uint16_t noisePerMS = (int)msg[16];
    noisePerMS += ((int)msg[17]) << 8;
    noise = noisePerMS;

    // agc
    uint16_t agcCnt = (int)msg[18];
    agcCnt += ((int)msg[19]) << 8;
    agc = agcCnt;

    uint8_t flags = (int)msg[22];

    // meaning of columns:
    // 01 21 - signature of NAV-TIMEUTC message
    // second in current week, year, month, day, hour, minute, seconds(+fraction)
    // accuracy (ns)
  //   cout<<"0a 09 "<<dec<<noisePerMS<<" "<<agcCnt<<" "<<flush;
  //   cout<<endl;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-MON-HW message:" << endl;
        tempStream << " noise            : " << dec << noisePerMS << " dBc" << endl;
        tempStream << " agcCnt (0..8192) : " << dec << agcCnt << endl;
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "   RTC calibrated   : " << string((flags & 1) ? "yes" : "no") << endl;
        tempStream << "   safe boot        : " << string((flags & 2) ? "yes" : "no") << endl;
        tempStream << "   jamming state    : " << (int)((flags & 0x0c) >> 2) << endl;
        tempStream << "   Xtal absent      : " << string((flags & 0x10) ? "yes" : "no");
        //tempStream >> temp;
        tempStream << "\n";
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}


void QtSerialUblox::UBXMonTx(const std::string& msg)
{
    // parse all fields
    // nr bytes pending
    uint16_t pending[6];
    uint8_t usage[6];
    uint8_t peakUsage[6];
    uint8_t tUsage;
    uint8_t tPeakUsage;
    //uint8_t errors;

    for (int i = 0; i < 6; i++) {
        pending[i] = msg[2 * i];
        pending[i] |= (uint16_t)msg[2 * i + 1] << 8;
        usage[i] = msg[i + 12];
        peakUsage[i] = msg[i + 18];
    }

    tUsage = msg[24];
    tPeakUsage = msg[25];
    //errors = msg[26];

    // meaning of columns:
    // 01 21 - signature of NAV-TIMEUTC message
    // second in current week, year, month, day, hour, minute, seconds(+fraction)
    // accuracy (ns)
  //   cout<<"0a 09 "<<dec<<noisePerMS<<" "<<agcCnt<<" "<<flush;
  //   cout<<endl;
    //mutex.lock();
    emit gpsPropertyUpdatedUint8(tUsage,txBufUsage.updateAge(),'b');
    emit gpsPropertyUpdatedUint8(tPeakUsage, txBufPeakUsage.updateAge(), 'p');
    txBufUsage = tUsage;
    txBufPeakUsage = tPeakUsage;
    //mutex.unlock();

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << setfill(' ') << setw(3);
        tempStream << "*** UBX-MON-TXBUF message:" << endl;
        tempStream << " global TX buf usage      : " << dec << (int)tUsage << " %" << endl;
        tempStream << " global TX buf peak usage : " << dec << (int)tPeakUsage << " %" << endl;
        tempStream << " TX buf usage for target      : ";
        for (int i = 0; i < 6; i++) {
            tempStream << "    (" << dec << i << ") " << setw(3) << (int)usage[i];
        }
        tempStream << endl;
        tempStream << " TX buf peak usage for target : ";
        for (int i = 0; i < 6; i++) {
            tempStream << "    (" << dec << i << ") " << setw(3) << (int)peakUsage[i];
        }
        tempStream << endl;
        tempStream << " TX bytes pending for target  : ";
        for (int i = 0; i < 6; i++) {
            tempStream << "    (" << dec << i << ") " << setw(3) << pending[i];
        }
        tempStream << "\n";
        //tempStream << setw(1) << setfill(' ');
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
}
