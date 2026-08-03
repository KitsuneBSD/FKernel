#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

// setreuid(ruid, euid): musl's seteuid() calls this as setreuid(-1, euid)
extern "C" uint64_t sys_setreuid(uint64_t ruid, uint64_t euid, uint64_t, uint64_t, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  if (ruid != (uint64_t)-1) {
    if (t->control.identity.euid != 0 && t->control.identity.uid != (uint32_t)ruid)
      return fkernel::return_error(fk::core::Error::PermissionDenied);
    t->control.identity.uid = (uint32_t)ruid;
  }
  if (euid != (uint64_t)-1) {
    if (t->control.identity.euid != 0 && t->control.identity.uid != (uint32_t)euid)
      return fkernel::return_error(fk::core::Error::PermissionDenied);
    t->control.identity.euid = (uint32_t)euid;
  }
  return 0;
}
