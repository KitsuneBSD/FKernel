#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockInterrupt.h>
#include <Kernel/Posix/errno.h>
#include <Kernel/Posix/sys/time.h>

namespace {
bool is_leap(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

time_t datetime_to_epoch(const DateTime &dt) {
  if (dt.year < 1970) {
    return 0;
  }

  static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30,
                                      31, 31, 30, 31, 30, 31};
  long days = 0;
  for (int y = 1970; y < dt.year; ++y) {
    days += is_leap(y) ? 366 : 365;
  }

  for (int m = 1; m < dt.month; ++m) {
    days += days_in_month[m];
    if (m == 2 && is_leap(dt.year)) {
      days++;
    }
  }

  days += dt.day - 1;

  return days * 86400L + dt.hour * 3600L + dt.minute * 60L + dt.second;
}
} // namespace

extern "C" {
int gettimeofday(struct timeval *tv, struct timezone *tz) {
  if (!tv) {
    errno = EFAULT;
    return -1;
  }

  auto current_time = ClockManager::the().datetime();
  tv->tv_sec = datetime_to_epoch(current_time);
  tv->tv_usec = 0; // RTC does not provide microsecond precision

  if (tz) {
    // We don't support timezones yet.
    tz->tz_minuteswest = 0;
    tz->tz_dsttime = 0;
  }

  return 0;
}
}
