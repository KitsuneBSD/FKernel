#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

// sys_setuid(...) → 0 or -errno
extern "C" uint64_t sys_setuid(uint64_t uid, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  if (t->control.identity.euid != 0 && t->control.identity.uid != (uint32_t)uid)
    return fkernel::return_error(fk::core::Error::PermissionDenied);
  const bool was_root = (t->control.identity.euid == 0);
  t->control.identity.euid = (uint32_t)uid;
  if (was_root)
    t->control.identity.uid = (uint32_t)uid;
  return 0;
}
