#include "components/log_parameter_processor.h"

#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "data/custom_io_operators.h"
#include "data/events/bias_switch_event.h"
#include "data/events/event_trigger_event.h"
#include "data/events/gain_switch_event.h"
#include "data/events/mcp4728_event.h"
#include "data/events/polarity_switch_event.h"
#include "data/events/preamp_switch_event.h"
#include "data/events/threshold_setting_event.h"
#include "data/ublox/ublox_messages.h"
#include "data/ublox/ublox_structs.h"
#include "utility/logparameter.h"

#include <format>
#include <string>

void LogParameterProcessor::setup(EventBus& bus) {
    bus.subscribe<UbxTimeMarkStruct>([&bus](const UbxTimeMarkStruct& tm) {
        if (!tm.risingValid && !tm.fallingValid) {
            logDebug("Daemon::onUBXReceivedTimeTM2(const UbxTimeMarkStruct&): detected invalid "
                     "time mark message; no rising or falling edge data");
            return;
        }
        // static UbxTimeMarkStruct lastTimeMark {};
        using namespace std::chrono;
        auto systime{system_clock::now().time_since_epoch()};
        auto gpstime{
            duration_cast<nanoseconds>(nanoseconds(tm.rising.tv_nsec) + seconds(tm.rising.tv_sec))};
        auto difftime{systime - gpstime};
        // std::cout<<std::dec<<"Tdiff: "<<difftime.count()*1e-9<<" s\n";
        bus.publish(LogParameter("gnssTimeOffset",
                                 std::format("{:.3f}", difftime.count() * 1e-9) + " s",
                                 LogParameter::LOG_LATEST));

        // long double dts = (tm.falling.tv_sec - tm.rising.tv_sec) * 1.0e9L;
        // dts += (tm.falling.tv_nsec - tm.rising.tv_nsec);
        // if ((dts > 0.0L) && tm.fallingValid) {
        //     m_histo_map["UbxEventLength"]->fill(static_cast<double>(dts));
        // }
        // long double interval = (tm.rising.tv_sec - lastTimeMark.rising.tv_sec) * 1.0e9L;
        // interval += (tm.rising.tv_nsec - lastTimeMark.rising.tv_nsec);
        // if (interval < 1e12)
        //     m_histo_map["UbxEventInterval"]->fill(static_cast<double>(1.0e-6L * interval));
        // uint16_t diffCount = tm.evtCounter - lastTimeMark.evtCounter;
        // emit timeMarkIntervalCountUpdate(diffCount, static_cast<double>(interval * 1.0e-9L));
        // lastTimeMark = tm;

        // output is: rising falling timeAcc valid timeBase utcAvailable
        std::stringstream tempStream;
        tempStream << tm.rising << tm.falling << tm.accuracy_ns << " " << tm.evtCounter << " "
                   << static_cast<short>(tm.valid) << " " << static_cast<short>(tm.timeBase) << " "
                   << static_cast<short>(tm.utcAvailable);
        bus.publish(LogParameter("ubloxCounter", std::to_string(tm.evtCounter) + " ",
                                 LogParameter::LOG_LATEST));

        if (!tm.risingValid || !tm.fallingValid) {
            logDebug("detected timemark message with reconstructed edge time (" +
                     std::string((tm.risingValid) ? "falling" : "rising") +
                     ")\nmsg: " + tempStream.str());
        }
    });
}

void LogParameterProcessor::poll(EventBus& bus, const DataStore& datastore) {
}