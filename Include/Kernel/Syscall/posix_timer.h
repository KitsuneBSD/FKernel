#pragma once

#include <Kernel/Scheduler/Task/task.h>

// Shared POSIX timer state used by the sys_timer_* syscalls (syscall_list/Time/)
// and the scheduler tick path (Scheduler/scheduler_lifecycle.cpp).
// The single definition lives in Src/Kernel/Syscall/syscall_list/Time/posix_timer.cpp.
struct PosixTimer {
  bool used{false};
  int signo{14}; // SIGALRM default
  uint64_t interval_ticks{0};
  uint64_t expiry_ticks{0};
  Task* owner{nullptr};
};

inline constexpr int MAX_POSIX_TIMERS = 8;

extern PosixTimer s_timers[MAX_POSIX_TIMERS];
