#ifndef DAC_THRESHOLD_H
#define DAC_THRESHOLD_H

#include "containers/message_container.h"

class dac_threshold : public message_container{
public:
    dac_threshold(uint8_t f_channel, float f_threshold);
    auto byte_array() -> std::string override;
    auto json() -> std::string override;

private:
    uint8_t m_channel;
    float m_threshold;
};

#endif // DAC_THRESHOLD_H