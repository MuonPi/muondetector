#ifndef DATABASELOGSINK_H
#define DATABASELOGSINK_H

#include "abstractsink.h"

#include <memory>

namespace MuonPi {

class ClusterLog;
class DatabaseLink;

/**
 * @brief The DatabaseLogSink class
 */
class DatabaseLogSink : public AbstractSink<ClusterLog>
{
public:
	/**
     * @brief DatabaseEventSink
	 * @param link a DatabaseLink instance
     */
    DatabaseLogSink(DatabaseLink& link);

protected:
	/**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
	void process(ClusterLog log);
	DatabaseLink& m_link;
};

}

#endif // DATABASELOGSINK_H
