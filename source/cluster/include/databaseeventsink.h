#ifndef DATABASEEVENTSINK_H
#define DATABASEEVENTSINK_H

#include "abstractsink.h"

#include <memory>

namespace MuonPi {

class Event;
class DatabaseLink;

/**
 * @brief The DatabaseEventSink class
 */
class DatabaseEventSink : public AbstractSink<Event>
{
public:
    /**
     * @brief DatabaseEventSink
     */
    DatabaseEventSink(DatabaseLink& link);
protected:


    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
    void process(Event event);

    DatabaseLink& m_link;
};

}

#endif // DATABASEEVENTSINK_H
