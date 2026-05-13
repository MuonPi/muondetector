#ifndef SERVER_CONN_COUNT_EVENT_H
#define SERVER_CONN_COUNT_EVENT_H

#include <cstdint>

struct ServerConnCountEvent {
    std::size_t n_sessions{0};
    bool newlyConnected{true};
};

#endif // SERVER_CONN_COUNT_EVENT_H