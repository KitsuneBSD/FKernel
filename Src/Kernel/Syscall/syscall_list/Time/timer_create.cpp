#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Syscall/posix_timer.h>
#include <LibFK/Utilities/memory.h>

// sys_timer_create(...) → 0 or -errno
extern "C" uint64_t sys_timer_create(uint64_t clockid, uint64_t sigevent_ptr, uint64_t timerid_ptr,
                          uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  (void)clockid;
  auto* out = reinterpret_cast<int*>(timerid_ptr);
  if (!out)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  for (int i = 0; i < MAX_POSIX_TIMERS; ++i) {
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
