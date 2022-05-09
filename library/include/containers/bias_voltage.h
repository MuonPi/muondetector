#ifndef BIAS_VOLTAGE_H
#define BIAS_VOLTAGE_H

#include "containers/message_container.h"

class bias_voltage : public message_container {
public:
    bias_voltage(float f_voltage);
    auto byte_array() -> std::string override;
    auto json() -> std::string override;
private:
    float m_voltage;
};

#endif // BIAS_VOLTAGE_H