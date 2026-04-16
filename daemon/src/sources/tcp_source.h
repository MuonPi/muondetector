#ifndef TCP_SOURCE_H
#define TCP_SOURCE_H

#include "core/event_bus.h"
#include "sources/source.h"

#include <memory>

class TcpConnection;

class TcpSource : public Source
{
public:
    explicit TcpSource(ComponentId id, EventBus& bus);

    void registerConnection(const std::shared_ptr<TcpConnection>& connection);
    void update() override;

private:
    EventBus& bus_;
};

#endif // TCP_SOURCE_H
