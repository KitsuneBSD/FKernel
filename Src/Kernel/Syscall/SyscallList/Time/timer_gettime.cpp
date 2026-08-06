#include <LibFK/Synchronization/interrupt_disabler.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Syscall/posix_timer.h>

static constexpr uint32_t TICKS_PER_SEC = 1000;

// sys_timer_gettime(...) → 0 or -errno
extern "C" uint64_t sys_timer_gettime(uint64_t timerid, uint64_t curr_value_ptr, uint64_t,
                           uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int id = (int)timerid;
  fk::synchronization::ScopedLockIRQ lock(s_timer_lock);
  if (id < 0 || id >= (int)s_timers.size() || !s_timers[id].used)
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
