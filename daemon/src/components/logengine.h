#ifndef LOGENGINE_H
#define LOGENGINE_H

#include "config.h"
#include "core/event_bus.h"
#include "sources/source.h"
#include "utility/logparameter.h"

#include <list>
#include <string>
#include <unordered_map>

class LogEngine : public Source {

  public:
    LogEngine(ComponentId id, EventBus& bus);
    ~LogEngine();

    void onLogParameterReceived(const LogParameter& logpar);
    void update();
    void onOnceLogTrigger() { onceLogFlag = true; }

  private:
    std::unordered_map<std::string, std::list<LogParameter>> logData;
    bool onceLogFlag = true;
    EventBus& bus_;
};

#endif // LOGENGINE_H
