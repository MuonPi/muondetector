#include <i2cproperty.h>


I2cProperty::I2cProperty(int pcaChannel, float dac_Thresh1, float dac_Thresh2, float biasVoltage, bool biasPowerOn){
    pcaChann = pcaChannel;
    thresh1 = dac_Thresh1;
    thresh2 = dac_Thresh2;
    bias_Voltage = biasVoltage;
    bias_powerOn = biasPowerOn;
}

QDataStream &operator<<(QDataStream &in, I2cProperty &property) {
    in << property.pcaChann << property.thresh1 << property.thresh2 << property.bias_Voltage << property.bias_powerOn;
    return in;
}

QDataStream &operator>>(QDataStream &out, I2cProperty &property) {
    out >> property.pcaChann >> property.thresh1 >> property.thresh2 >> property.bias_Voltage >> property.bias_powerOn;
    return out;
}
