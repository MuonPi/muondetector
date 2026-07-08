#ifndef SINK_H
#define SINK_H

class Sink {
  public:
    virtual void shutdown() {};
    virtual ~Sink() = default;
};

#endif // SINK_H
