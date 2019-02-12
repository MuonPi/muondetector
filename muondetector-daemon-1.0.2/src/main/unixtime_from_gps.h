#ifndef UNIXTIME_FROM_GPS_H
#define UNIXTIME_FROM_GPS_H

#include <chrono>
#include <time_from_rtc.h>
#include <QDateTime>

static timespec unixtime_from_gps(int week_nr, long int s_of_week, long int ns /*,int leap_seconds*/) {
    QDateTime gpsTimeStart(QDate(1980,1,6),QTime(0,0,0,0));
    quint64 gpsOffsetToEpoch = gpsTimeStart.toMSecsSinceEpoch()/1000;
    //qDebug() << "week: " << week_nr << " second: " << s_of_week << " ns: " << ns;
    //qDebug() << "gpsOffsetToEpoch: "<<gpsOffsetToEpoch;
    quint64 secs = gpsOffsetToEpoch;
    secs += week_nr*7*24*3600;  // 7 days per week, 24 hours per day and 3600 seconds per hour
    secs += s_of_week;
    //qDebug() << "secs: " << secs;
    //qDebug() << "secs: " << secs;
    /*
	struct tm time;
	time.tm_year = 80;
	time.tm_mon = 0;
	time.tm_mday = 6;
	time.tm_isdst = -1;
	time.tm_hour = 0;
	time.tm_min = 0;
	time.tm_sec = 0;

	int GpsCycle = 0;

	long GpsDays = 1024 * 7;
	time.tm_mday += GpsDays;
	mktime(&time);
    GpsDays = ((GpsCycle * 1024) + (week_nr - 1024)) * 7 + (s_of_week / 86400L);
	//  time.tm_mday+=10000;
	time.tm_mday += GpsDays;
	mktime(&time);

    long int sod = s_of_week - ((long int)(s_of_week / 86400L)) * 86400L;
	//  printf("s of d: %ld\n",msod/1000);
	time.tm_hour = sod / 3600;
	time.tm_min = sod / 60 - time.tm_hour * 60;
	time.tm_sec = sod - time.tm_hour * 3600 - time.tm_min * 60/*+leap_seconds*/;
    /*mktime(&time);


	t_from_rtc(&time);

	time_t secs = t_from_rtc(&time);

	//   printf("GPS time: %s",asctime(&time));
*/
	struct timespec ts;
	ts.tv_sec = secs;
    ts.tv_nsec = ns;
	//   cout<<secs<<"."<<setw(6)<<setfill('0')<<ns<<" "<<setfill(' ')<<endl;

	return ts;
}

#endif // UNIXTIME_FROM_GPS_H
