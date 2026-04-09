#ifndef DAEMON_H
#define DAEMON_H

#include "core/thread_pool.h"
#include "core/scheduler.h"
#include "core/event_bus.h"
#include "core/system_builder.h"


struct SystemConfig;
class Daemon
{
public:
    Daemon(const SystemConfig& f_config);
    ~Daemon();

    void exec();
    void stop();

private:
    void initScheduledTasks();

    const SystemConfig& m_config;
    ThreadPool m_pool;
    Scheduler m_scheduler;
    EventBus m_bus;
    std::atomic<bool> m_running{true};
};

#endif // DAEMON_H