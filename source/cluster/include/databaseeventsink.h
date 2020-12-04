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
protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
    std::unique_ptr<DatabaseLink> m_link { nullptr };
};

}

#endif // DATABASEEVENTSINK_H
