#ifndef MINIX_TIME_H
#define MINIX_TIME_H

typedef long time_t;
typedef long clock_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

time_t time(time_t *timer);
int stime(const time_t *timer);

struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
char *asctime(const struct tm *timeptr);
char *ctime(const time_t *timer);
double difftime(time_t end, time_t beginning);
time_t mktime(struct tm *timeptr);

#endif /* MINIX_TIME_H */
