#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Syscall/posix_timer.h>
#include <LibFK/Utilities/memory.h>

// sys_timer_getoverrun(...) → 0 or -errno
extern "C" uint64_t sys_timer_getoverrun(uint64_t timerid, uint64_t, uint64_t,
                               uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int id = (int)timerid;
  if (id < 0 || id >= MAX_POSIX_TIMERS || !s_timers[id].used)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  return 0;
}
