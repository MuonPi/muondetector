#ifndef TCP_SINK_H
#define TCP_SINK_H

#include "sink.h"
#include "data/ad1115_event.h"
#include "tcpconnection.h"

#include <vector>
#include <mutex>


class TcpSink : public Sink
{
public:
    void handle(const Ad1115SampleEvent& event);
    void addConnection(std::shared_ptr<TcpConnection> conn);

private:
    std::vector<uint8_t> serialize(const Ad1115SampleEvent& event);

private:
    std::vector<std::shared_ptr<TcpConnection>> connections_;
    std::mutex mutex_;
};
#endif // TCP_SINK_H
