#include "daemon.h"
#include "app/system_config.h"
#include "core/event_bus.h"
#include "core/scheduler.h"
#include "core/thread_pool.h"

#include <chrono>
#include <memory>

using namespace std::chrono_literals;

Daemon::Daemon(const SystemConfig& f_config)
    : m_config(f_config)
    , m_pool([&]() -> std::size_t {
          std::size_t n = 0;

          n = m_config.max_thread_count;

          if (n == 0)
          {
              n = std::thread::hardware_concurrency();
          }

          if (n == 0)
          {
              n = 1; // absolute fallback
          }

          n = std::min<std::size_t>(n, 64);

          return n;
      }())
    , m_scheduler(m_pool)
    , m_bus(m_pool)
{
}

Daemon::~Daemon()
{
}

void Daemon::stop()
{
    m_running = false;
}


void Daemon::exec()
{
    // Creates all sinks and sources and initializes all sensors
    auto context = SystemBuilder::build(m_pool, m_config, m_scheduler);

    // worker
    m_scheduler.start();

    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    m_scheduler.stop();
}