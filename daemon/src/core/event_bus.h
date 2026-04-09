#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "thread_pool.h"

#include <functional>
#include <unordered_map>
#include <typeindex>


class EventBus {
public:
    explicit EventBus(ThreadPool &pool);
    ~EventBus();

    template<typename T>
    void publish(const T& event);

    template<typename T>
    void subscribe(std::function<void(const T&)> handler);

private:
    std::unordered_map<std::type_index, std::vector<std::function<void(const void*)>>> subscribers;
    ThreadPool& threadPool;
};

template<typename T>
void EventBus::publish(const T& event)
{
    auto it = subscribers.find(typeid(T));
    if (it == subscribers.end()) return;

    for (auto& handler : it->second)
    {
        T copy = event;

        threadPool.enqueue([handler, copy]() mutable {
            handler(&copy);
        });
    }
}
template<typename T>
void EventBus::subscribe(std::function<void(const T&)> handler)
{
    subscribers[typeid(T)].push_back(
        [handler](const void* e)
        {
            handler(*static_cast<const T*>(e));
        });
}

#endif // EVENT_BUS_H