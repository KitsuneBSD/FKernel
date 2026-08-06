#include <LibFK/Synchronization/interrupt_disabler.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Syscall/posix_timer.h>

extern "C" uint64_t sys_timer_create(uint64_t clockid, uint64_t sigevent_ptr, uint64_t timerid_ptr,
                          uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  (void)clockid;
  auto* out = reinterpret_cast<int*>(timerid_ptr);
  if (!out)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  int signo = 14; // SIGALRM default
  if (sigevent_ptr) {
    auto* se = reinterpret_cast<const uint32_t*>(sigevent_ptr);
    int s = (int)se[1];
    if (s > 0 && s < 32) signo = s;
  }
  Task* owner = SchedulerManager::the().current();

  fk::synchronization::ScopedLockIRQ lock(s_timer_lock);

  // Reuse a free slot to keep timer IDs stable and bounded in common case
  for (size_t i = 0; i < s_timers.size(); ++i) {
    if (!s_timers[i].used) {
      s_timers[i].used = true;
      s_timers[i].interval_ticks = 0;
      s_timers[i].expiry_ticks = 0;
      s_timers[i].signo = signo;
      s_timers[i].owner = owner;
      *out = (int)i;
      return 0;
    }
  }

  // No free slot — grow the vector
  PosixTimer t;
  t.used = true;
  t.interval_ticks = 0;
  t.expiry_ticks = 0;
  t.signo = signo;
  t.owner = owner;
  TRY_OR_FATAL(s_timers.push_back(fk::types::move(t)));
  *out = (int)(s_timers.size() - 1);
  return 0;
}
