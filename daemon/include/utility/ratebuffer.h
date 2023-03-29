#ifndef RATEBUFFER_H
#define RATEBUFFER_H
#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <chrono>
#include <limits>
#include <list>
#include <map>
#include <muondetector_structs.h>
#include <queue>
#include <utility/gpio_mapping.h>

using namespace std::literals;

constexpr double MAX_AVG_RATE { 100. };
constexpr unsigned long MAX_BURST_MULTIPLICITY { 10 };
constexpr std::chrono::microseconds MAX_BUFFER_TIME { 60s };
constexpr std::chrono::microseconds MAX_DEADTIME { static_cast<unsigned long>(1e+6 / MAX_AVG_RATE) };
constexpr std::chrono::microseconds DEADTIME_INCREMENT { 50 };

class CounterRateBuffer : public QObject {
    Q_OBJECT

public:
    CounterRateBuffer(unsigned int counter_mask = static_cast<unsigned int>(std::numeric_limits<std::uint16_t>::max()), QObject* parent = nullptr);
    ~CounterRateBuffer() = default;
    void clear();

    [[nodiscard]] auto avgRate() const -> double;
    [[nodiscard]] auto lastEventTime() const -> EventTime;

signals:

public slots:
    void onCounterValue(uint16_t value);

private:
    unsigned int m_counter_mask {};
    std::chrono::microseconds m_buffer_time { MAX_BUFFER_TIME };
    std::list<std::pair<EventTime, std::uint16_t>> m_countbuffer {};
    std::chrono::nanoseconds m_last_interval { 0 };
    EventTime m_instance_start {};
};

class EventRateBuffer : public QObject {
    Q_OBJECT

public:
    /*
    struct RateItem {
        double avg_rate { 0. };
        double std_dev { 0. };
        std::chrono::milliseconds interval {};
        EventTime time {};
        std::chrono::microseconds deadtime {};
    };
    */
    EventRateBuffer(unsigned int gpio, QObject* parent = nullptr);
    ~EventRateBuffer() = default;
    void setRateLimit(double max_cps);
    [[nodiscard]] auto currentRateLimit() const -> double { return fRateLimit; }
    void clear();

    [[nodiscard]] auto avgRate() const -> double;
    [[nodiscard]] auto currentDeadtime() const -> std::chrono::microseconds;
    [[nodiscard]] auto lastInterval() const -> std::chrono::nanoseconds;
    [[nodiscard]] auto lastEventTime() const -> EventTime;

signals:
    void filteredEvent(uint8_t gpio, EventTime event_time);
    void eventIntervalSignal(uint8_t gpio, std::chrono::nanoseconds ns);

public slots:
    //void onEvent(unsigned int gpio, EventTime event_time);
    void onEvent(uint8_t gpio);

private:
    double fRateLimit { MAX_AVG_RATE };
    uint8_t m_gpio { 255 };
    std::chrono::microseconds m_buffer_time { MAX_BUFFER_TIME };
    std::queue<EventTime, std::list<EventTime>> m_eventbuffer {};
    std::chrono::microseconds m_current_deadtime { 0 };
    std::chrono::nanoseconds m_last_interval { 0 };
    EventTime m_instance_start {};
};

#endif // RATEBUFFER_H
