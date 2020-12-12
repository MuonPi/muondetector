#ifndef ASCIIEVENTSINK_H
#define ASCIIEVENTSINK_H

#include "abstractsink.h"

#include <memory>

namespace MuonPi {

class Event;
class CombinedEvent;

/**
 * @brief The AsciiEventSink class
 */
class AsciiEventSink : public AbstractSink<Event>
{
public:
    /**
     * @brief AsciiEventSink
     * @param publishers The topics from which the messages should be published
     */
    AsciiEventSink(std::ostream* a_ostream);

    ~AsciiEventSink() override;
protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:

    void process(std::unique_ptr<Event> evt);
    void process(std::unique_ptr<CombinedEvent> evt);

    std::ostream* m_ostream { nullptr };
};

}

#endif // ASCIIEVENTSINK_H
