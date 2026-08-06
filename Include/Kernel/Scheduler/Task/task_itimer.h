#pragma once

#include <LibFK/Types/types.h>

struct TaskITimer {
  uint64_t remaining_ticks{0};
  uint64_t interval_ticks{0};
  int signo{14};
  bool active{false};
};
