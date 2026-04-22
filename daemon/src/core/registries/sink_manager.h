#ifndef SINK_MANAGER_H
#define SINK_MANAGER_H

#include "sinks/sink.h"

#include <memory>
#include <vector>

class SinkManager {
  public:
    void add(std::shared_ptr<Sink> src);

  private:
    std::vector<std::shared_ptr<Sink>> m_sinks;
};

#endif // SINK_MANAGER_H