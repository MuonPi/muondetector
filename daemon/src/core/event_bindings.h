#ifndef EVENT_BINDINGS_H
#define EVENT_BINDINGS_H

#include "core/event_bus.h"
#include "core/registries/component_manager.h"
#include "core/registries/data_store.h"
#include "data/events/datastore_store_event.h"
#include "sinks/mqtt_sink.h"
#include "sinks/tcp_sink.h"

#include <array>
#include <type_traits>

class EventBindings {
  public:
    // make tcp sink send data through tcp connections
    static void setupTcpSink(EventBus& bus, TcpSink& tcp_sink);

    static void setupMqttSink(EventBus& bus, MqttSink& mqtt_sink);

    // All Events are either published wrapped inside of DataStoreStoreEvent or "normally"
    // If published wrapped, can also contain arrays of Events (for example for threshold events)
    // The array is stored in storage as std::array<event, x> and fan out here by iterating
    // So both stored and not stored events are always sent out to tcp connection
    // IMPORTANT: Never publish both DatastoreStoreEvent<some_event> and some_event because it will
    // be published twice to GUI.
    template <typename T, typename = void>
    struct is_iterable : std::false_type {};

    template <typename T>
    struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T>())),
                                      decltype(std::end(std::declval<T>()))>> : std::true_type {};

    template <typename T>
    inline static void subscribe_one(EventBus& bus, DataStore& datastore) {
        bus.subscribe<DatastoreStoreEvent<T>>([&datastore, &bus](const DatastoreStoreEvent<T>& ev) {
            datastore.store(ev.data);
            if constexpr (is_iterable<T>::value && !std::is_same_v<T, std::string>) {
                for (const auto& item : ev.data) {
                    bus.publish(item);
                }
            } else {
                bus.publish(ev.data);
            }
        });
    }

    template <typename... Ts>
    inline static void subscribe_all(EventBus& bus, DataStore& datastore) {
        (subscribe_one<Ts>(bus, datastore), ...);
    }
    static void pollDatastore(EventBus& bus, DataStore& datastore);
    static void setupDatastore(EventBus& bus, DataStore& datastore);
    static void setupLogging(EventBus& bus, DataStore& datastore, ComponentManager& components);

  private:
    EventBindings() = delete;
    static void processGeoPos(EventBus& bus, DataStore& datastore, const GeoPosition& pos);
};
#endif // EVENT_BINDINGS_H
