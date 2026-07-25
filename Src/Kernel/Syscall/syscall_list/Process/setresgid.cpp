#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {

uint64_t sys_setresgid(uint64_t rgid, uint64_t egid, uint64_t sgid,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  auto& id = t->control.identity;
  auto can_set = [&](uint32_t val) {
    if (id.euid == 0) return true;
    return val == (uint32_t)-1 || val == id.gid || val == id.egid || val == id.sgid;
  };
  if (!can_set((uint32_t)rgid) || !can_set((uint32_t)egid) || !can_set((uint32_t)sgid))
    return fkernel::return_error(fk::core::Error::PermissionDenied);
  if (rgid != (uint64_t)-1) id.gid  = (uint32_t)rgid;
  if (egid != (uint64_t)-1) id.egid = (uint32_t)egid;
  if (sgid != (uint64_t)-1) id.sgid = (uint32_t)sgid;
  return 0;
}

uint64_t sys_getresgid(uint64_t rgid_ptr, uint64_t egid_ptr, uint64_t sgid_ptr,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  auto& id = t->control.identity;
  if (rgid_ptr) *reinterpret_cast<uint32_t*>(rgid_ptr) = id.gid;
  if (egid_ptr) *reinterpret_cast<uint32_t*>(egid_ptr) = id.egid;
  if (sgid_ptr) *reinterpret_cast<uint32_t*>(sgid_ptr) = id.sgid;
  return 0;
}

}
