#ifndef STATUS_H
#define STATUS_H

#include "containers/message_container.h"
#include <map>

class status : public message_container {
public:
    enum class status_field
    {
        bias,
        gain,
        preamp1,
        preamp2,
        polarity,
        mqtt
    };
    
    using status_map = std::map<status_field, bool>;

    status(const status_map &f_status_map);
    auto byte_array() -> std::string override;
    auto json() -> std::string override;

private:
    status_map m_status_map;
};

#endif // STATUS_H