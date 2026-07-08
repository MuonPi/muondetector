#include "sink_manager.h"

void SinkManager::add(std::shared_ptr<Sink> src) {
    m_sinks.push_back(std::move(src));
}

void SinkManager::shutdownAll() {
    for (auto& sink : m_sinks) {
        if (sink) {
            sink->shutdown();
        }
    }
}
