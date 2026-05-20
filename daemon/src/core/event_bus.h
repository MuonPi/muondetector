#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "thread_pool.h"

#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>

class EventBus {
  public:
    explicit EventBus(ThreadPool& pool);
    ~EventBus();

    template <typename T>
    void publish(T&& event);

    template <typename T>
    void subscribe(std::function<void(const T&)> handler);

  private:
    std::mutex mutex;
    std::unordered_map<std::type_index, std::vector<std::function<void(const void*)>>> subscribers;
    ThreadPool& threadPool;
};

template <typename T>
void EventBus::publish(T&& event) {
    using EventType = std::decay_t<T>;

    std::vector<std::function<void(const void*)>> handlers;

    {
        std::scoped_lock lock{mutex};

        auto it = subscribers.find(typeid(EventType));
        if (it == subscribers.end())
            return;

        handlers = it->second; // snapshot
    }

    EventType event_copy = std::forward<T>(event);

    threadPool.enqueue(
        [handlers = std::move(handlers), event_copy = std::move(event_copy)]() mutable {
            for (auto& handler : handlers) {
                handler(&event_copy);
            }
        });
}

template <typename T>
void EventBus::subscribe(std::function<void(const T&)> handler) {
    std::lock_guard<std::mutex> lock{mutex};
    subscribers[typeid(T)].push_back(
        [handler](const void* e) { handler(*static_cast<const T*>(e)); });
}

#endif // EVENT_BUS_H