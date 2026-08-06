#pragma once

#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Synchronization/spinlock.h>

// Shared POSIX timer state used by the sys_timer_* syscalls (SyscallList/Time/)
// and the scheduler tick path (Scheduler/scheduler_lifecycle.cpp).
// Definitions live in Src/Kernel/Syscall/SyscallList/Time/posix_timer.cpp.
struct PosixTimer {
  bool used{false};
  int signo{14}; // SIGALRM default
  uint64_t interval_ticks{0};
  uint64_t expiry_ticks{0};
  Task* owner{nullptr};
};

extern fk::containers::Vector<PosixTimer> s_timers;
extern fk::synchronization::Spinlock s_timer_lock;
