#ifndef TEMPERATURE_EVENT_H
#define TEMPERATURE_EVENT_H

struct TemperatureEvent {
    std::string source{"unknown"};
    float temperature{-1.0};
};

#endif // TEMPERATURE_EVENT_H