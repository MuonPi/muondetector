#include "pigpiodhandler.h"
#include "utility/gpio_mapping.h"
#include <QDebug>
#include <QPointer>
#include <cmath>
#include <config.h>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <sys/time.h>
#include <time.h>

constexpr char CONSUMER[] = "muonpi";
const std::string chipname { "/dev/gpiochip0" };

template <typename T>
inline static T sqr(T x) { return x * x; }

/*
 linear regression algorithm
 taken from:
   http://stackoverflow.com/questions/5083465/fast-efficient-least-squares-fit-algorithm-in-c
 parameters:
  n = number of data points
  xarray,yarray  = arrays of data
  *offs = output intercept
  *slope  = output slope
 */
void calcLinearCoefficients(int n, quint64* xarray, qint64* yarray,
    double* offs, double* slope, int arrayIndex = 0)
{
    if (n < 3)
        return;

    long double sumx = 0.0L; /* sum of x                      */
    long double sumx2 = 0.0L; /* sum of x**2                   */
    long double sumxy = 0.0L; /* sum of x * y                  */
    long double sumy = 0.0L; /* sum of y                      */
    long double sumy2 = 0.0L; /* sum of y**2                   */

    int ix = arrayIndex;
    if (ix == 0)
        ix = n - 1;
    else
        ix--;

    quint64 offsx = xarray[ix];
    qint64 offsy = yarray[ix];

    int i;
    for (i = 0; i < n; i++) {
        sumx += xarray[i] - offsx;
        sumx2 += sqr(xarray[i] - offsx);
        sumxy += (xarray[i] - offsx) * (yarray[i] - offsy);
        sumy += (yarray[i] - offsy);
        sumy2 += sqr(yarray[i] - offsy);
    }

    double denom = (n * sumx2 - sqr(sumx));
    if (denom == 0) {
        // singular matrix. can't solve the problem.
        *slope = 0.;
        *offs = 0.;
        return;
    }

    long double m = (n * sumxy - sumx * sumy) / denom;
    long double b = (sumy * sumx2 - sumx * sumxy) / denom;

    *slope = (double)m;
    *offs = (double)(b + offsy);
}

PigpiodHandler::PigpiodHandler(std::vector<unsigned int> gpioPins, QObject* parent)
    : QObject(parent)
{
    startOfProgram = QDateTime::currentDateTimeUtc();
    lastSamplingTime = startOfProgram;
    elapsedEventTimer.start();

    fChip = gpiod_chip_open(chipname.c_str());
    if (fChip == nullptr) {
        qCritical() << "error opening gpio chip " << QString::fromStdString(chipname);
        throw std::exception();
        return;
    }
    isInitialised = true;

    for (unsigned int gpioPin : gpioPins) {
        gpiod_line* line = gpiod_chip_get_line(fChip, gpioPin);
        int flags = 0;
        /*
		GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN |
		GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE |
		GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
		*/
        int ret = gpiod_line_request_rising_edge_events_flags(line, CONSUMER, flags);
        if (ret < 0) {
            qCritical() << "Request for registering gpio line" << gpioPin << "for event notification failed";
            continue;
        }
        fInterruptLineMap.emplace(std::make_pair(gpioPin, line));
    }

    gpioClockTimeMeasurementTimer.setInterval(MuonPi::Config::Hardware::GPIO::Clock::Measurement::interval /*GPIO_CLOCK_MEASUREMENT_INTERVAL_MS*/);
    gpioClockTimeMeasurementTimer.setSingleShot(false);
    connect(&gpioClockTimeMeasurementTimer, &QTimer::timeout, this, &PigpiodHandler::measureGpioClockTime);
    gpioClockTimeMeasurementTimer.start();

    reloadInterruptSettings();
}

PigpiodHandler::~PigpiodHandler()
{
    this->stop();
    for (auto [gpio, line] : fInterruptLineMap) {
        gpiod_line_release(line);
    }
    for (auto [gpio, line] : fLineMap) {
        gpiod_line_release(line);
    }
    if (fChip != nullptr)
        gpiod_chip_close(fChip);
}

void PigpiodHandler::processEvent(unsigned int gpio, std::shared_ptr<gpiod_line_event> line_event)
{

    std::chrono::nanoseconds since_epoch_ns(line_event->ts.tv_nsec);
    since_epoch_ns += std::chrono::seconds(line_event->ts.tv_sec);
    EventTime timestamp(since_epoch_ns);
    emit event(gpio, timestamp);
    if (verbose > 3) {
        qDebug() << "line event: gpio" << gpio << " edge: "
                 << QString((line_event->event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? "rising" : "falling")
                 << " ts=" << since_epoch_ns.count();
    }
}

void PigpiodHandler::eventHandler(struct gpiod_line* line)
{
    static const struct timespec timeout {
        4ULL, 0ULL
    };
    while (fThreadRunning) {
        if (inhibit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        const unsigned int gpio = gpiod_line_offset(line);
        std::shared_ptr<gpiod_line_event> line_event { std::make_shared<gpiod_line_event>() };
        int ret = gpiod_line_event_wait(line, &timeout);
        if (ret > 0) {
            int read_result = gpiod_line_event_read(line, line_event.get());
            if (read_result == 0) {
                std::thread process_event_bg(&PigpiodHandler::processEvent, this, gpio, std::move(line_event));
                process_event_bg.detach();
            } else {
                // an error occured
                // what should we do here?
                qCritical() << "read gpio line event failed";
            }
        } else if (ret == 0) {
            // a timeout occurred, no event was detected
            // simply go over into the wait loop again
        } else {
            // an error occured
            // what should we do here?
            qCritical() << "wait for gpio line event failed";
        }
    }
}

bool PigpiodHandler::setPinInput(unsigned int gpio)
{
    if (!isInitialised)
        return false;
    auto it = fLineMap.find(gpio);

    if (it != fLineMap.end()) {
        // line object exists, request for input
        int ret = gpiod_line_request_input(it->second, CONSUMER);
        if (ret < 0) {
            qCritical() << "Request gpio line" << gpio << "as input failed";
            return false;
        }
        return true;
    }
    // line was not allocated yet, so do it now
    gpiod_line* line = gpiod_chip_get_line(fChip, gpio);
    if (line == nullptr) {
        qCritical() << "error allocating gpio line" << gpio;
        return false;
    }
    int ret = gpiod_line_request_input(line, CONSUMER);
    if (ret < 0) {
        qCritical() << "Request gpio line" << gpio << "as input failed";
        return false;
    }
    fLineMap.emplace(std::make_pair(gpio, line));
    return true;
}

bool PigpiodHandler::setPinOutput(unsigned int gpio, bool initState)
{
    if (!isInitialised)
        return false;
    auto it = fLineMap.find(gpio);

    if (it != fLineMap.end()) {
        // line object exists, request for output
        int ret = gpiod_line_request_output(it->second, CONSUMER, static_cast<int>(initState));
        if (ret < 0) {
            qCritical() << "Request gpio line" << gpio << "as output failed";
            return false;
        }
        return true;
    }
    // line was not allocated yet, so do it now
    gpiod_line* line = gpiod_chip_get_line(fChip, gpio);
    if (line == nullptr) {
        qCritical() << "error allocating gpio line" << gpio;
        return false;
    }
    int ret = gpiod_line_request_output(line, CONSUMER, static_cast<int>(initState));
    if (ret < 0) {
        qCritical() << "Request gpio line" << gpio << "as output failed";
        return false;
    }
    fLineMap.emplace(std::make_pair(gpio, line));
    return true;
}

bool PigpiodHandler::setPinBias(unsigned int gpio, std::uint8_t bias_flags)
{
    if (!isInitialised)
        return false;
    int flags = 0;
    if (bias_flags & PinBias::OpenDrain) {
        flags |= GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN;
    } else if (bias_flags & PinBias::OpenSource) {
        flags |= GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE;
    } else if (bias_flags & PinBias::ActiveLow) {
        flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
    } else if (bias_flags & PinBias::PullDown) {
        //flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
    } else if (bias_flags & PinBias::PullUp) {
        //flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
    } else if (!(bias_flags & PinBias::PullDown) && !(bias_flags & PinBias::PullUp)) {
        //flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLED;
    }

    auto it = fLineMap.find(gpio);
    int dir = -1;
    if (it != fLineMap.end()) {
        // line object exists, set config
        dir = gpiod_line_direction(it->second);
        gpiod_line_release(it->second);
        fLineMap.erase(it);
    }
    // line was not allocated yet, so do it now
    gpiod_line* line = gpiod_chip_get_line(fChip, gpio);
    if (line == nullptr) {
        qCritical() << "error allocating gpio line" << gpio;
        return false;
    }
    int req_mode = GPIOD_LINE_REQUEST_DIRECTION_AS_IS;
    if (dir == GPIOD_LINE_DIRECTION_INPUT)
        req_mode = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
    else if (dir == GPIOD_LINE_DIRECTION_OUTPUT)
        req_mode = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
    gpiod_line_request_config config { CONSUMER, req_mode, flags };
    int ret = gpiod_line_request(line, &config, 0);
    if (ret < 0) {
        qCritical() << "Request gpio line" << gpio << "for bias config failed";
        return false;
    }
    fLineMap.emplace(std::make_pair(gpio, line));
    return true;
}

bool PigpiodHandler::setPinState(unsigned int gpio, bool state)
{
    if (!isInitialised)
        return false;
    auto it = fLineMap.find(gpio);
    if (it != fLineMap.end()) {
        // line object exists, request for output
        int ret = gpiod_line_set_value(it->second, static_cast<int>(state));
        if (ret < 0) {
            qCritical() << "Setting state of gpio line" << gpio << "failed";
            return false;
        }
        return true;
    }
    // line was not allocated yet, so do it now
    return setPinOutput(gpio, state);
}

void PigpiodHandler::reloadInterruptSettings()
{
    this->stop();
    this->start();
}

bool PigpiodHandler::registerInterrupt(unsigned int gpio, EventEdge edge)
{
    if (!isInitialised)
        return false;
    auto it = fInterruptLineMap.find(gpio);
    if (it != fInterruptLineMap.end()) {
        // line object exists
        // The following code block shall release the previous line request
        // and re-request this line for events.
        // It is bypassed, since it does not work as intended.
        // The function simply does nothing in this case.
        return false;
        //stop();
        //gpiod_line_release(it->second);
        if (gpiod_line_update(it->second) < 0) {
            qCritical() << "update of gpio line" << gpio << "after release failed";
            //return false;
        }
        // request for events
        int ret = -1;
        int errcnt = 10;
        while (errcnt && ret < 0) {
            switch (edge) {
            case EventEdge::RisingEdge:
                ret = gpiod_line_request_rising_edge_events(it->second, CONSUMER);
                break;
            case EventEdge::FallingEdge:
                ret = gpiod_line_request_falling_edge_events(it->second, CONSUMER);
                break;
            case EventEdge::BothEdges:
                ret = gpiod_line_request_both_edges_events(it->second, CONSUMER);
            default:
                break;
            }
            errcnt--;
            if (ret < 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (ret < 0) {
            qCritical() << "Re-request gpio line" << gpio << "for events failed";
            return false;
        }
        return true;
    }
    // line was not allocated yet, so do it now
    gpiod_line* line = gpiod_chip_get_line(fChip, gpio);
    if (line == nullptr) {
        qCritical() << "error allocating gpio line" << gpio;
        return false;
    }
    int ret = -1;
    switch (edge) {
    case EventEdge::RisingEdge:
        ret = gpiod_line_request_rising_edge_events(line, CONSUMER);
        break;
    case EventEdge::FallingEdge:
        ret = gpiod_line_request_falling_edge_events(line, CONSUMER);
        break;
    case EventEdge::BothEdges:
        ret = gpiod_line_request_both_edges_events(line, CONSUMER);
    default:
        break;
    }
    if (ret < 0) {
        qCritical() << "Request gpio line" << gpio << "for events failed";
        return false;
    }
    fInterruptLineMap.emplace(std::make_pair(gpio, line));
    reloadInterruptSettings();
    return true;
}

bool PigpiodHandler::unRegisterInterrupt(unsigned int gpio)
{
    if (!isInitialised)
        return false;
    auto it = fInterruptLineMap.find(gpio);
    if (it != fInterruptLineMap.end()) {
        gpiod_line_release(it->second);
        fInterruptLineMap.erase(it);
        reloadInterruptSettings();
        return true;
    }
    return false;
}

bool PigpiodHandler::initialised()
{
    return isInitialised;
}

void PigpiodHandler::stop()
{
    fThreadRunning = false;
    for (auto& [gpio, line_thread] : fThreads) {
        if (line_thread)
            line_thread->join();
    }
}

void PigpiodHandler::start()
{
    if (fThreadRunning)
        return;
    fThreadRunning = true;

    for (auto& [gpio, line] : fInterruptLineMap) {
        fThreads[gpio] = std::make_unique<std::thread>([this, gpio, line]() { this->eventHandler(line); });
    }
}

void PigpiodHandler::measureGpioClockTime()
{
    if (!isInitialised)
        return;
    static uint32_t oldTick = 0;
    //    static uint64_t llTick = 0;
    const int N = MuonPi::Config::Hardware::GPIO::Clock::Measurement::buffer_size /*GPIO_CLOCK_MEASUREMENT_BUFFER_SIZE*/;
    static int nrSamples = 0;
    static int arrayIndex = 0;
    static qint64 diff_array[N];
    static quint64 tick_array[N];
    struct timespec tp, tp1, tp2;

    quint64 t0 = startOfProgram.toMSecsSinceEpoch();

    clock_gettime(CLOCK_REALTIME, &tp1);
    ///    uint32_t tick = get_current_tick(pi);
    uint32_t tick = 0;
    clock_gettime(CLOCK_REALTIME, &tp2);
    //        clock_gettime(CLOCK_MONOTONIC, &tp);

    qint64 dt = tp2.tv_sec - tp1.tv_sec;
    dt *= 1000000000LL;
    dt += (tp2.tv_nsec - tp1.tv_nsec);
    dt /= 2000;

    tp = tp1;

    if (tick < oldTick) {
        gpioTickOverflowCounter = gpioTickOverflowCounter + UINT32_MAX + 1;
    }
    oldTick = tick;
    quint64 nr_usecs = ((quint64)tp.tv_sec * 1000 - t0) * 1000;
    nr_usecs += tp.tv_nsec / 1000 + dt;
    diff_array[arrayIndex] = (qint64)(nr_usecs - gpioTickOverflowCounter) - tick;
    tick_array[arrayIndex] = (quint64)gpioTickOverflowCounter + tick;
    lastTimeMeasurementTick = (quint64)gpioTickOverflowCounter + tick;
    if (++arrayIndex >= N) {
        arrayIndex = 0;
    }
    if (nrSamples < N)
        nrSamples++;

    double offs = 0.;
    double slope = 0.;
    calcLinearCoefficients(nrSamples, tick_array, diff_array, &offs, &slope, arrayIndex);
    clockMeasurementSlope = slope;
    clockMeasurementOffset = offs;

    //	qDebug() << "gpio clock measurement: N=" << nrSamples << " offs=" << offs << " slope=" << slope;
}
