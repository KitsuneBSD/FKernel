#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {

uint64_t sys_setgid(uint64_t gid, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
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

// setregid(rgid, egid): musl's setegid() calls this as setregid(-1, egid)
uint64_t sys_setregid(uint64_t rgid, uint64_t egid, uint64_t, uint64_t, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  if (rgid != (uint64_t)-1) {
    if (t->control.identity.euid != 0 && t->control.identity.gid != (uint32_t)rgid)
      return fkernel::return_error(fk::core::Error::PermissionDenied);
    t->control.identity.gid = (uint32_t)rgid;
  }
  if (egid != (uint64_t)-1) {
    if (t->control.identity.euid != 0 && t->control.identity.gid != (uint32_t)egid)
      return fkernel::return_error(fk::core::Error::PermissionDenied);
    t->control.identity.egid = (uint32_t)egid;
  }
  return 0;
}

}
