#ifndef TIME_FROM_RTC_H
#define TIME_FROM_RTC_H

#include <chrono>


static time_t t_from_rtc(struct tm *stm) {
    bool rtc_on_utc = true;

    struct tm temp1, temp2;
    long diff;
    time_t t1, t2;

    temp1 = *stm;
    temp1.tm_isdst = 0;

    t1 = mktime(&temp1);
    if (rtc_on_utc) {
        temp2 = *gmtime(&t1);
    }
    else {
        temp2 = *localtime(&t1);
    }

    temp2.tm_isdst = 0;
    t2 = mktime(&temp2);
    diff = t2 - t1;

    if (t1 - diff == -1)
        //DEBUG_LOG(LOGF_RtcLinux, "Could not convert RTC time");
        printf("Could not convert RTC time");
    return t1 - diff;
}

#endif // TIME_FROM_RTC_H
