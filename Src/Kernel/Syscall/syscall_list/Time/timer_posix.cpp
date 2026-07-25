#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

// POSIX timer implementation: timer_create allocates a slot; timer_settime
// programs the timer; on_tick() decrements expiry_ticks and delivers the
// signal when it reaches zero.

static constexpr int MAX_TIMERS = 8;
static constexpr uint32_t TICKS_PER_SEC = 1000;

struct PosixTimer {
  bool used{false};
  int signo{14}; // SIGALRM default
  uint64_t interval_ticks{0};
  uint64_t expiry_ticks{0};
  Task* owner{nullptr};
};

// Exported for scheduler tick processing
PosixTimer s_timers[MAX_TIMERS];

extern "C" {

uint64_t sys_timer_create(uint64_t clockid, uint64_t sigevent_ptr, uint64_t timerid_ptr,
                          uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  (void)clockid;
  auto* out = reinterpret_cast<int*>(timerid_ptr);
  if (!out)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  for (int i = 0; i < MAX_TIMERS; ++i) {
    if (!s_timers[i].used) {
      s_timers[i].used = true;
      s_timers[i].interval_ticks = 0;
      s_timers[i].expiry_ticks = 0;
      s_timers[i].signo = 14; // SIGALRM
      s_timers[i].owner = SchedulerManager::the().current();

      // Parse sigevent if provided
      if (sigevent_ptr) {
        // struct sigevent { int sigev_notify; int sigev_signo; ... }
        auto* se = reinterpret_cast<const uint32_t*>(sigevent_ptr);
        int signo = (int)se[1]; // sigev_signo at offset 1
        if (signo > 0 && signo < 32) s_timers[i].signo = signo;
      }

      *out = i;
      return 0;
    }
  }
  return fkernel::return_error(fk::core::Error::OutOfMemory);
}

uint64_t sys_timer_delete(uint64_t timerid, uint64_t, uint64_t,
                          uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int id = (int)timerid;
  if (id < 0 || id >= MAX_TIMERS || !s_timers[id].used)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  s_timers[id].used = false;
  return 0;
}

uint64_t sys_timer_settime(uint64_t timerid, uint64_t flags, uint64_t new_value_ptr,
                           uint64_t old_value_ptr, uint64_t, uint64_t,
                           [[maybe_unused]] PtRegs* regs) {
  (void)flags;
  int id = (int)timerid;
  if (id < 0 || id >= MAX_TIMERS || !s_timers[id].used)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto& timer = s_timers[id];

  // Report old value
  if (old_value_ptr) {
    // itimerspec: { it_interval (2x uint64), it_value (2x uint64) } = 32 bytes
    auto* ov = reinterpret_cast<uint64_t*>(old_value_ptr);
    if (timer.expiry_ticks > 0) {
      uint64_t remaining_us = (timer.expiry_ticks * 1000000) / TICKS_PER_SEC;
      ov[2] = remaining_us / 1000000;        // it_value.tv_sec
      ov[3] = remaining_us % 1000000;        // it_value.tv_nsec (using tv_usec slot)
    } else {
      ov[2] = 0; ov[3] = 0;
    }
    uint64_t int_us = (timer.interval_ticks * 1000000) / TICKS_PER_SEC;
    ov[0] = int_us / 1000000;               // it_interval.tv_sec
    ov[1] = int_us % 1000000;               // it_interval.tv_nsec
  }

  // Set new value
  if (new_value_ptr) {
    auto* nv = reinterpret_cast<const uint64_t*>(new_value_ptr);
    uint64_t value_sec = nv[2];
    uint64_t value_nsec = nv[3];
    uint64_t int_sec = nv[0];
    uint64_t int_nsec = nv[1];

    uint64_t value_us = value_sec * 1000000 + value_nsec / 1000;
    uint64_t int_us = int_sec * 1000000 + int_nsec / 1000;

    timer.expiry_ticks = (value_us * TICKS_PER_SEC) / 1000000;
    timer.interval_ticks = (int_us * TICKS_PER_SEC) / 1000000;
  }

  return 0;
}

uint64_t sys_timer_gettime(uint64_t timerid, uint64_t curr_value_ptr, uint64_t,
                           uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int id = (int)timerid;
  if (id < 0 || id >= MAX_TIMERS || !s_timers[id].used)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  if (curr_value_ptr) {
    auto* cv = reinterpret_cast<uint64_t*>(curr_value_ptr);
    auto& timer = s_timers[id];
    if (timer.expiry_ticks > 0) {
      uint64_t remaining_us = (timer.expiry_ticks * 1000000) / TICKS_PER_SEC;
      cv[2] = remaining_us / 1000000;
      cv[3] = remaining_us % 1000000;
    } else {
      cv[2] = 0; cv[3] = 0;
    }
    uint64_t int_us = (timer.interval_ticks * 1000000) / TICKS_PER_SEC;
    cv[0] = int_us / 1000000;
    cv[1] = int_us % 1000000;
  }
  return 0;
}

uint64_t sys_timer_getoverrun(uint64_t timerid, uint64_t, uint64_t,
                               uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int id = (int)timerid;
  if (id < 0 || id >= MAX_TIMERS || !s_timers[id].used)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  return 0;
}

}
