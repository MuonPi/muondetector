#ifndef I2CPROPERTY_H
#define I2CPROPERTY_H
#include <QDataStream>
class I2cProperty{
public:
    I2cProperty(int pcaChannel = -1, float dac_Thresh1 = -1, float dac_Thresh2 = -1, float biasVoltage = -1, bool biasPowerOn = false);
    int pcaChann;
    float thresh1, thresh2, bias_Voltage;
    bool bias_powerOn;
    friend QDataStream& operator<<(QDataStream& in, I2cProperty& property);
    friend QDataStream& operator>>(QDataStream& out, I2cProperty& property);
};

#endif // I2CPROPERTY_H
