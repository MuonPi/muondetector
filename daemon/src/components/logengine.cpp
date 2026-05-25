#include "components/logengine.h"

#include "core/event_bus.h"
#include "data/events/file_message_event.h"
#include "data/events/log_trigger_event.h"
#include "data/events/mqtt_log_event.h"
#include "logparameter.h"
#include "utility/conversion.h"

#include <chrono>
#include <config.h>
#include <format>
#include <string>
#include <string_view>

static inline std::string_view last_token(std::string_view s) {
    size_t pos = s.find_last_of(' ');
    if (pos == std::string_view::npos)
        return s;
    return s.substr(pos + 1);
}

LogEngine::LogEngine(EventBus& bus) : bus_(bus) {
    bus_.subscribe<LogTriggerEvent>(
        [this]([[maybe_unused]] const LogTriggerEvent&) { onLogInterval(); });
}

LogEngine::~LogEngine() {
}

void LogEngine::onLogParameterReceived(const LogParameter& logpar) {
    if (logpar.logType() == LogParameter::LOG_NEVER) {
        return;
    }
    if (logpar.logType() == LogParameter::LOG_EVERY) {
        // directly log to file since LOG_EVERY attribute is set
        // no need to store in buffer, just return after logging

        std::string msg{dateStringNow() + " " + logpar.name() + " " + logpar.value()};
        bus_.publish(MqttLogEvent{.msg = msg});
        bus_.publish(FileMessageEvent{.msg = msg});
        // reset already existing entries but preserve logType attribute
        if (logData.find(logpar.name()) != logData.end()) {
            int logType = logData[logpar.name()].front().logType();
            if (logType != LogParameter::LOG_AVERAGE) {
                logData[logpar.name()].clear();
            }
            logData[logpar.name()].push_back(LogParameter(logpar.name(), logpar.value(), logType));
            logData[logpar.name()].back().setUpdatedRecently(false);
        }
        return;
    } else {
        // save to buffer
        if (logpar.logType() == LogParameter::LOG_ONCE) {
            // don't save if a LOG_ONCE param with this name is already in buffer
        }
        logData[logpar.name()].push_back(logpar);
        logData[logpar.name()].back().setUpdatedRecently(true);
    }
}

void LogEngine::onLogInterval() {
    // Use one timestamp for writing all parameters
    // Otherwise log messages can spread over multiple seconds, making data import challenging
    auto ts = dateStringNow();

    // loop over the map with all accumulated parameters since last log reminder
    // no increment here since we erase and invalidate iterators within the loop
    for (auto it = logData.begin(); it != logData.end();) {
        // check if name string is set but no entry exists. This should not happen
        auto& name = it->first;
        auto& list = it->second;
        if (list.empty()) {
            ++it;
            continue;
        }

        if (list.back().logType() == LogParameter::LOG_LATEST) {
            // easy to write only the last value to file
            std::string msg{ts + " " + name + " " + list.back().value()};
            bus_.publish(MqttLogEvent{.msg = msg});
            bus_.publish(FileMessageEvent{.msg = std::move(msg)});
            it = logData.erase(it);
        } else if (list.back().logType() == LogParameter::LOG_AVERAGE) {

            double sum = 0.0;
            bool ok = true;

            auto value = list.back().value();
            std::string_view unitString = last_token(value);

            auto firstToken = [](const std::string& s) -> std::string {
                auto pos = s.find(' ');
                return (pos == std::string::npos) ? s : s.substr(0, pos);
            };

            for (const auto& p : list) {

                const auto& v = p.value();
                std::string numStr = firstToken(v);

                try {
                    sum += std::stod(numStr);
                } catch (...) {
                    ok = false;
                    break;
                }
            }

            if (ok && !list.empty()) {
                sum /= list.size();

                std::string msg = ts + " " + std::format("{} {:.7f} {}", name, sum, unitString);

                bus_.publish(MqttLogEvent{.msg = msg});
                bus_.publish(FileMessageEvent{.msg = std::move(msg)});
            }

            it = logData.erase(it);
        } else if (list.back().logType() == LogParameter::LOG_ONCE) {
            // we want to log only one time per daemon lifetime || file change
            if (onceLogFlag || list.front().updatedRecently()) {
                std::string msg{ts + " " + name + " " + list.back().value()};
                bus_.publish(MqttLogEvent{.msg = msg});
                bus_.publish(FileMessageEvent{.msg = std::move(msg)});
            }
            while (list.size() > 2) {
                list.pop_front();
            }
            list.front().setUpdatedRecently(false);
            logData[name] = list;
            ++it;
        } else if (list.back().logType() == LogParameter::LOG_ON_CHANGE) {
            // we want to log only if one value differs from the first entry
            // first entry is reference value
            auto it_first = list.begin();

            // log first time or on update
            if (onceLogFlag || it_first->updatedRecently()) {

                std::string msg = ts + " " + name + " " + list.back().value();

                bus_.publish(MqttLogEvent{.msg = msg});
                bus_.publish(FileMessageEvent{.msg = std::move(msg)});
            } else {

                for (auto it = std::next(list.begin()); it != list.end(); ++it) {

                    if (it->value() != it_first->value()) {

                        std::string msg = ts + " " + name + " " + it->value();

                        bus_.publish(MqttLogEvent{.msg = msg});
                        bus_.publish(FileMessageEvent{.msg = std::move(msg)});

                        // replace reference value
                        *it_first = *it;

                        break; // optional but usually intended
                    }
                }
            }

            while (list.size() > 1) {
                list.pop_back();
            }

            list.front().setUpdatedRecently(false);
            ++it;
        } else
            ++it;
    }
    onceLogFlag = false;
}
