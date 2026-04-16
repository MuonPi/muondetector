/*
    This class defines which data types are stored in the datastore
    by defining the required overloaded function to handle the data type.
*/
#ifndef DATASTORE_SINK_H
#define DATASTORE_SINK_H

#include "sinks/sink.h"
#include "data/events/ubx_event.h"
#include "datastore/datastore.h"
#include <mutex>


class DatastoreSink : public Sink
{
public:
    DatastoreSink(Datastore& datastore);
    void handle(const UbxEvent& event);

private:
    Datastore& datastore_;
};

#endif // DATASTORE_SINK_H