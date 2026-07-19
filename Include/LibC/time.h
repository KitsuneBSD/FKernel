#pragma once

#ifdef __GLIBC__
#include <time.h>
#else

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long time_t;
typedef long clock_t;
typedef long suseconds_t;

#define CLOCKS_PER_SEC 1000000L

#define CLOCK_REALTIME           0
#define CLOCK_MONOTONIC          1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3
#define CLOCK_MONOTONIC_RAW      4
#define CLOCK_REALTIME_COARSE    5
#define CLOCK_MONOTONIC_COARSE   6
#define CLOCK_BOOTTIME           7

struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};

struct timeval {
    time_t      tv_sec;
    suseconds_t tv_usec;
};

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

time_t    time(time_t *tloc);
int       clock_gettime(int clk_id, struct timespec *tp);
int       clock_settime(int clk_id, const struct timespec *tp);
int       clock_getres(int clk_id, struct timespec *res);
double    difftime(time_t t1, time_t t0);
struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
time_t    mktime(struct tm *tm);
size_t    strftime(char *s, size_t max, const char *format, const struct tm *tm);
int       nanosleep(const struct timespec *req, struct timespec *rem);
int       usleep(unsigned int usec);
int       sleep(unsigned int sec);

#ifdef __cplusplus
}
#endif

#endif // __GLIBC__
