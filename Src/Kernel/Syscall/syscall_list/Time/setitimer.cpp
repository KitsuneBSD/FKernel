#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

struct timeval {
  long tv_sec;
  long tv_usec;
};

struct itimerval {
  timeval it_interval;
  timeval it_value;
};

static constexpr uint32_t TICKS_PER_SEC = 1000; // 1000 Hz tick

extern "C" uint64_t sys_setitimer(uint64_t which, uint64_t new_val,
                                   uint64_t old_val, uint64_t, uint64_t, uint64_t,
                                   [[maybe_unused]] PtRegs* regs) {
  int w = (int)which;
  if (w < 0 || w > 2) return (uint64_t)-22; // EINVAL

  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  auto& timer = task->control.lifecycle.itimers[w];

  // Report old value
  if (old_val) {
    auto* ov = reinterpret_cast<itimerval*>(old_val);
    if (timer.active && timer.remaining_ticks > 0) {
      uint64_t remaining_us = (timer.remaining_ticks * 1000000) / TICKS_PER_SEC;
      ov->it_value.tv_sec = remaining_us / 1000000;
      ov->it_value.tv_usec = remaining_us % 1000000;
    } else {
      ov->it_value = {0, 0};
    }
    uint64_t int_us = (timer.interval_ticks * 1000000) / TICKS_PER_SEC;
    ov->it_interval.tv_sec = int_us / 1000000;
    ov->it_interval.tv_usec = int_us % 1000000;
  }

  // Set new value
  if (new_val) {
    auto* nv = reinterpret_cast<const itimerval*>(new_val);
    uint64_t value_us = (uint64_t)nv->it_value.tv_sec * 1000000 + (uint64_t)nv->it_value.tv_usec;
    uint64_t interval_us = (uint64_t)nv->it_interval.tv_sec * 1000000 + (uint64_t)nv->it_interval.tv_usec;

    timer.remaining_ticks = (value_us * TICKS_PER_SEC) / 1000000;
    timer.interval_ticks = (interval_us * TICKS_PER_SEC) / 1000000;
    timer.active = (timer.remaining_ticks > 0);
    timer.signo = 14; // SIGALRM
  }

  return 0;
}

extern "C" uint64_t sys_getitimer(uint64_t which, uint64_t curr_val,
                                   uint64_t, uint64_t, uint64_t, uint64_t,
                                   [[maybe_unused]] PtRegs* regs) {
  int w = (int)which;
  if (w < 0 || w > 2) return (uint64_t)-22;

  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  auto& timer = task->control.lifecycle.itimers[w];

  if (curr_val) {
    auto* cv = reinterpret_cast<itimerval*>(curr_val);
    if (timer.active && timer.remaining_ticks > 0) {
      uint64_t remaining_us = (timer.remaining_ticks * 1000000) / TICKS_PER_SEC;
      cv->it_value.tv_sec = remaining_us / 1000000;
      cv->it_value.tv_usec = remaining_us % 1000000;
    } else {
      cv->it_value = {0, 0};
    }
    uint64_t int_us = (timer.interval_ticks * 1000000) / TICKS_PER_SEC;
    cv->it_interval.tv_sec = int_us / 1000000;
    cv->it_interval.tv_usec = int_us % 1000000;
  }
  return 0;
}
