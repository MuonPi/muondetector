#ifndef LOGENGINE_H
#define LOGENGINE_H

#include "config.h"
#include "core/event_bus.h"
#include "utility/logparameter.h"

#include <list>
#include <string>
#include <unordered_map>

class LogEngine {

  public:
    LogEngine(EventBus& bus);
    ~LogEngine();

    void sendLogString(const std::string& str);
    void logIntervalSignal();

    void onLogParameterReceived(const LogParameter& logpar);
    void onLogInterval();
    void onOnceLogTrigger() { onceLogFlag = true; }

  private:
    std::unordered_map<std::string, std::list<LogParameter>> logData;
    bool onceLogFlag = true;
    EventBus& bus_;
};

#endif // LOGENGINE_H
