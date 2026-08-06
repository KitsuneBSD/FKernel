#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

// sys_getresuid(...) → 0 or -errno
extern "C" uint64_t sys_getresuid(uint64_t ruid_ptr, uint64_t euid_ptr, uint64_t suid_ptr,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  auto& id = t->control.identity;
  if (ruid_ptr) *reinterpret_cast<uint32_t*>(ruid_ptr) = id.uid;
  if (euid_ptr) *reinterpret_cast<uint32_t*>(euid_ptr) = id.euid;
  if (suid_ptr) *reinterpret_cast<uint32_t*>(suid_ptr) = id.suid;
  return 0;
}
