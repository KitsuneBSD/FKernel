#pragma once

#include <LibFK/Types/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long time_t;
typedef long suseconds_t;

struct timeval {
  time_t tv_sec;       /* seconds */
  suseconds_t tv_usec; /* microseconds */
};

struct timezone {
  int tz_minuteswest; /* minutes west of Greenwich */
  int tz_dsttime;     /* type of DST correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifdef __cplusplus
}
#endif
