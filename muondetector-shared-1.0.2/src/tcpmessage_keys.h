#ifndef TCPMESSAGE_KEYS_H
#define TCPMESSAGE_KEYS_H
#include <muondetector_shared_global.h>

// no specific reason but the codes are all prime numbers :)
static const quint16 ping = 2;
static const quint16 quitConnectionSig = 3;
static const quint16 timeoutSig = 5;
static const quint16 threshSig = 7;
static const quint16 threshRequestSig = 11;
static const quint16 ubxMsgRateRequest = 13;
static const quint16 ubxMsgRate = 17;
static const quint16 gpioPinSig = 19;
static const quint16 biasVoltageSig = 23;
static const quint16 biasVoltageRequestSig = 29;
static const quint16 biasSig = 31;
static const quint16 biasRequestSig = 37;
static const quint16 pcaChannelSig = 41;
static const quint16 pcaChannelRequestSig = 43;
static const quint16 gpioRateRequestSig = 47;
static const quint16 gpioRateSig = 53;
static const quint16 gpioRateSettings = 59;
static const quint16 geodeticPosSig = 61;
static const quint16 adcSampleSig = 67;
static const quint16 adcSampleRequestSig = 71;
static const quint16 dacReadbackSig = 73;
static const quint16 dacRequestSig = 79;
static const quint16 gainSwitchSig = 83;
static const quint16 gainSwitchRequestSig = 89;
static const quint16 preampSig = 97;
static const quint16 preampRequestSig = 101;
static const quint16 temperatureSig = 103;
static const quint16 temperatureRequestSig = 107;
static const quint16 calibSetSig = 127;
static const quint16 calibRequestSig = 131;
static const quint16 i2cStatsSig = 137;
static const quint16 i2cStatsRequestSig = 139;
static const quint16 gpsSatsSig = 149;
static const quint16 calibWriteEepromSig = 151;
static const quint16 gpsTimeAccSig = 157;
static const quint16 i2cScanBusRequestSig = 163;
static const quint16 gpsTxBufSig = 167;
static const quint16 gpsTxBufPeakSig = 173;
static const quint16 gpsMonHWSig = 179;
static const quint16 gpsVersionSig = 181;
static const quint16 gpsFixSig = 191;
static const quint16 gpsIntCounterSig = 193;
static const quint16 ubxResetSig = 197;
static const quint16 ubxConfigureDefaultSig = 199;
// not implemented from here on yet
static const quint16 dacSetEepromSig = 109;
static const quint16 dacRequestEepromSig = 113;



// next prime numbers: 61 67 71 73 79 83 89 97 101 103 107 109 113 127 131 137 139 149 151 157 163 167 173 179 181 191 193 197 199 211


#endif // TCPMESSAGE_KEYS_H
