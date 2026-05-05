#ifndef TCP_COMMAND_DECODER_H
#define TCP_COMMAND_DECODER_H

#include "core/component.h"
#include "core/event_bus.h"
#include "data/events/tcp_packet_event.h"

class TcpCommandDecoder : public Component {
  public:
    TcpCommandDecoder(ComponentId id, EventBus& bus);

  private:
    void handle(const TcpPacketEvent& event);

    EventBus& bus_;
};

#endif // TCP_COMMAND_DECODER_H
