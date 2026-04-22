#include "sinks/datastore_sink.h"

#include "data/events/ubx_event.h"

#include <typeindex>

DatastoreSink::DatastoreSink(Datastore& datastore) : datastore_{datastore} {};

void DatastoreSink::handle(const UbxEvent& event) {
    datastore_.store<UbxEvent>(event);
}