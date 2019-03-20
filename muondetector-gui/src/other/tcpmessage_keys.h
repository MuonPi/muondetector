#ifndef TCPMESSAGE_KEYS_H
#define TCPMESSAGE_KEYS_H
#include <QtGlobal>

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

// next prime numbers: 61 67 71 73 79 83 89 97 101 103 107


#endif // TCPMESSAGE_KEYS_H
