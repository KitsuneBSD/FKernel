#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Scheduler/Core/scheduler.h>
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

// sys_getitimer(...) → 0 or -errno
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
