#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

static bool can_set(uint32_t val, uint32_t uid, uint32_t euid, uint32_t suid) {
  if (euid == 0) return true;
  return val == (uint32_t)-1 || val == uid || val == euid || val == suid;
}

// sys_setresuid(...) → 0 or -errno
extern "C" uint64_t sys_setresuid(uint64_t ruid, uint64_t euid, uint64_t suid,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  auto& id = t->control.identity;
  if (!can_set((uint32_t)ruid, id.uid, id.euid, id.suid) ||
      !can_set((uint32_t)euid, id.uid, id.euid, id.suid) ||
      !can_set((uint32_t)suid, id.uid, id.euid, id.suid))
    return fkernel::return_error(fk::core::Error::PermissionDenied);
  if (ruid != (uint64_t)-1) id.uid  = (uint32_t)ruid;
  if (euid != (uint64_t)-1) id.euid = (uint32_t)euid;
  if (suid != (uint64_t)-1) id.suid = (uint32_t)suid;
  return 0;
}
