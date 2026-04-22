#include "event_bus.h"

EventBus::EventBus(ThreadPool& pool) : threadPool(pool) {
}

EventBus::~EventBus() {
}
