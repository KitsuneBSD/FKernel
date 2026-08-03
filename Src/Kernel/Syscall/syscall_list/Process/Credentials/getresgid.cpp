#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

// sys_getresgid(...) → 0 or -errno
extern "C" uint64_t sys_getresgid(uint64_t rgid_ptr, uint64_t egid_ptr, uint64_t sgid_ptr,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  auto& id = t->control.identity;
  if (rgid_ptr) *reinterpret_cast<uint32_t*>(rgid_ptr) = id.gid;
  if (egid_ptr) *reinterpret_cast<uint32_t*>(egid_ptr) = id.egid;
  if (sgid_ptr) *reinterpret_cast<uint32_t*>(sgid_ptr) = id.sgid;
  return 0;
}
