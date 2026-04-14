#ifndef UNIXTIME_FROM_GPS_H
#define UNIXTIME_FROM_GPS_H

#include <ctime>
#include <cstdint>

static constexpr std::int64_t GPS_UNIX_EPOCH_OFFSET = 315964800LL;

static inline timespec unixtime_from_gps(
    int week_nr,
    std::int64_t s_of_week,
    std::int64_t ns)
{
    std::int64_t secs =
        GPS_UNIX_EPOCH_OFFSET +
        static_cast<std::int64_t>(week_nr) * 7LL * 24LL * 3600LL +
        s_of_week;

    timespec ts{};
    ts.tv_sec = static_cast<time_t>(secs);
    ts.tv_nsec = static_cast<long>(ns);

    return ts;
}

#endif // UNIXTIME_FROM_GPS_H