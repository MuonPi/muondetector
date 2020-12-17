#ifndef ABSTRACTEVENTSINK_H
#define ABSTRACTEVENTSINK_H

#include "threadrunner.h"

#include <future>
#include <memory>
#include <atomic>
#include <queue>

namespace MuonPi {

// +++ Forward declarations
class Event;
// --- Forward declarations

template <typename T>
/**
 * @brief The AbstractSink class
 * Represents a canonical Sink for items of type T.
 */
class AbstractSink : public ThreadRunner
{
public:
    AbstractSink();
    /**
     * @brief ~AbstractSink The destructor. If this gets called while the event loop is still running, it will tell the loop to finish and wait for it to be done.
     */
    virtual ~AbstractSink() override;

    /**
     * @brief push_item pushes an item into the sink
     * @param item The item to push
     */
    void push_item(const T& item);

protected:
    /**
      * @brief next_item gets the next available Event from the internal item buffer.
      * @return the next available Event if available, a nullptr otherwise.
      */
    [[nodiscard]] auto next_item() -> T;

    /**
     * @brief has_item Whether the source has items available
     * @return true if there are items to get
     */
    [[nodiscard]] auto has_items() const -> bool;

private:

    std::atomic<bool> m_has_items { false };

    std::queue<T> m_queue {};

    std::mutex m_mutex {};

};

template <typename T>
AbstractSink<T>::AbstractSink()
    : ThreadRunner{"AbstractSink"}
{}

template <typename T>
AbstractSink<T>::~AbstractSink() = default;

template <typename T>
auto AbstractSink<T>::next_item() -> T
{
    if (!has_items()) {
        return {};
    }
    std::scoped_lock<std::mutex> lock {m_mutex};
    auto item {std::move(m_queue.front())};
    m_queue.pop();
    if (m_queue.size() == 0) {
        m_has_items = false;
    }
    return item;
}

template <typename T>
void AbstractSink<T>::push_item(const T& item)
{
    std::scoped_lock<std::mutex> lock {m_mutex};
    m_queue.push(item);
    m_has_items = true;
}

template <typename T>
auto AbstractSink<T>::has_items() const -> bool
{
    return m_has_items;
}

}

#endif // ABSTRACTEVENTSINK_H
