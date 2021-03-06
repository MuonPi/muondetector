#ifndef TCPMESSAGE_KEYS_H
#define TCPMESSAGE_KEYS_H

#include "muondetector_shared_global.h"

// no specific reason but the codes are all prime numbers :)
enum class TCP_MSG_KEY : quint16
{ 	MSG_PING = 2,
	MSG_QUIT_CONNECTION = 3,
	MSG_TIMEOUT = 5,
	MSG_THRESHOLD = 7,
	MSG_THRESHOLD_REQUEST = 11,
	MSG_UBX_MSG_RATE_REQUEST = 13,
	MSG_UBX_MSG_RATE = 17,
	MSG_GPIO_EVENT = 19,
	MSG_BIAS_VOLTAGE = 23,
	MSG_BIAS_VOLTAGE_REQUEST = 29,
	MSG_BIAS_SWITCH = 31,
	MSG_BIAS_SWITCH_REQUEST = 37,
	MSG_PCA_SWITCH = 41,
	MSG_PCA_SWITCH_REQUEST = 43,
	MSG_GPIO_RATE_REQUEST = 47,
	MSG_GPIO_RATE = 53,
	MSG_GPIO_RATE_CFG = 59,
	MSG_GEO_POS = 61,
	MSG_ADC_SAMPLE = 67,
	MSG_ADC_SAMPLE_REQUEST = 71,
	MSG_DAC_READBACK = 73,
	MSG_DAC_REQUEST = 79,
	MSG_GAIN_SWITCH = 83,
	MSG_GAIN_SWITCH_REQUEST = 89,
	MSG_PREAMP_SWITCH = 97,
	MSG_PREAMP_SWITCH_REQUEST = 101,
	MSG_TEMPERATURE = 103,
	MSG_TEMPERATURE_REQUEST = 107,
	MSG_DAC_EEPROM_SET = 109,
	MSG_CALIB_SET = 127,
	MSG_CALIB_REQUEST = 131,
	MSG_I2C_STATS = 137,
	MSG_I2C_STATS_REQUEST = 139,
	MSG_GNSS_SATS = 149,
	MSG_CALIB_SAVE = 151,
	MSG_UBX_TIME_ACCURACY = 157,
	MSG_I2C_SCAN_BUS = 163,
	MSG_UBX_TXBUF = 167,
	MSG_UBX_TXBUF_PEAK = 173,
	MSG_UBX_MONHW = 179,
	MSG_UBX_VERSION = 181,
	MSG_UBX_FIXSTATUS = 191,
	MSG_UBX_EVENTCOUNTER = 193,
	MSG_UBX_RESET = 197,
	MSG_UBX_CONFIG_DEFAULT = 199,
	MSG_UBX_FREQ_ACCURACY = 211,
	MSG_UBX_MONHW2 = 213,
	MSG_UBX_GNSS_CONFIG = 221,
	MSG_UBX_UPTIME = 223,
	MSG_UBX_CFG_TP5 = 227,
	MSG_GPIO_RATE_RESET = 229,
	MSG_HISTOGRAM = 233,
	MSG_UBX_CFG_SAVE = 239,
	MSG_UBX_RXBUF = 241,
	MSG_UBX_RXBUF_PEAK = 251,
	MSG_EVENTTRIGGER = 257,
	MSG_EVENTTRIGGER_REQUEST = 263,
	MSG_SPI_STATS = 269,
	MSG_SPI_STATS_REQUEST = 271,
	MSG_HISTOGRAM_CLEAR = 277,
	MSG_ADC_TRACE = 281,
	MSG_ADC_MODE = 283,
	MSG_ADC_MODE_REQUEST = 293,
	MSG_LOG_INFO = 307,
	MSG_FILE_DOWNLOAD = 311,
	MSG_OLED_ENTRIES = 313,
	MSG_RATE_SCAN = 317,
    MSG_MQTT_STATUS = 331,
	MSG_GPIO_INHIBIT = 337,
	MSG_UBX_TIMEMARK = 349,
	MSG_POLARITY_SWITCH = 353,
	MSG_POLARITY_SWITCH_REQUEST = 359
};

/*
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
static const quint16 dacSetEepromSig = 109;
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
static const quint16 gpsFreqAccSig = 211;
static const quint16 gpsMonHW2Sig = 213;
static const quint16 gnssConfigSig = 221;
static const quint16 gpsUptimeSig = 223;
static const quint16 gpsCfgTP5Sig = 227;
static const quint16 resetRateSig = 229;
static const quint16 histogramSig = 233;
static const quint16 ubxSaveCfgSig = 239;
static const quint16 gpsRxBufSig = 241;
static const quint16 gpsRxBufPeakSig = 251;
static const quint16 eventTriggerSig = 257;
static const quint16 eventTriggerRequestSig = 263;
static const quint16 spiStatsSig = 269;
static const quint16 spiStatsRequestSig = 271;
static const quint16 histogramClearSig = 277;
static const quint16 adcTraceSig = 281;


// not implemented from here on yet
static const quint16 dacRequestEepromSig = 113;
*/

// list of prime numbers
//https://primes.utm.edu/lists/small/10000.txt

/*
 *    2      3      5      7     11     13     17     19     23     29
     31     37     41     43     47     53     59     61     67     71
     73     79     83     89     97    101    103    107    109    113
    127    131    137    139    149    151    157    163    167    173
    179    181    191    193    197    199    211    223    227    229
    233    239    241    251    257    263    269    271    277    281
    283    293    307    311    313    317    331    337    347    349
    353    359    367    373    379    383    389    397    401    409
    419    421    431    433    439    443    449    457    461    463
    467    479    487    491    499    503    509    521    523    541
    547    557    563    569    571    577    587    593    599    601
    607    613    617    619    631    641    643    647    653    659
    661    673    677    683    691    701    709    719    727    733
    739    743    751    757    761    769    773    787    797    809
    811    821    823    827    829    839    853    857    859    863
    877    881    883    887    907    911    919    929    937    941
    947    953    967    971    977    983    991    997   1009   1013
   1019   1021   1031   1033   1039   1049   1051   1061   1063   1069
   1087   1091   1093   1097   1103   1109   1117   1123   1129   1151
   1153   1163   1171   1181   1187   1193   1201   1213   1217   1223
   1229   1231   1237   1249   1259   1277   1279   1283   1289   1291
   1297   1301   1303   1307   1319   1321   1327   1361   1367   1373
   1381   1399   1409   1423   1427   1429   1433   1439   1447   1451
   1453   1459   1471   1481   1483   1487   1489   1493   1499   1511
   1523   1531   1543   1549   1553   1559   1567   1571   1579   1583
   1597   1601   1607   1609   1613   1619   1621   1627   1637   1657
   1663   1667   1669   1693   1697   1699   1709   1721   1723   1733
   1741   1747   1753   1759   1777   1783   1787   1789   1801   1811
   1823   1831   1847   1861   1867   1871   1873   1877   1879   1889
   1901   1907   1913   1931   1933   1949   1951   1973   1979   1987
   1993   1997   1999   2003   2011   2017   2027   2029   2039   2053
*/

#endif // TCPMESSAGE_KEYS_H
