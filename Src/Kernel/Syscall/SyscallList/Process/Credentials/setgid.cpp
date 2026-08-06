#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

// sys_setgid(...) → 0 or -errno
extern "C" uint64_t sys_setgid(uint64_t gid, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  if (t->control.identity.euid != 0 && t->control.identity.gid != (uint32_t)gid)
    return fkernel::return_error(fk::core::Error::PermissionDenied);
  t->control.identity.egid = (uint32_t)gid;
  if (t->control.identity.euid == 0)
    t->control.identity.gid = (uint32_t)gid;
  return 0;
}
