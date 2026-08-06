#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {

uint64_t sys_sigpending(uint64_t set_ptr, uint64_t, uint64_t,
                        uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  if (!set_ptr || !fkernel::memory::is_user_address(set_ptr, sizeof(uint64_t)))
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  uint64_t pending = t->resources.ipc.signals.pending;
  auto res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(set_ptr), &pending, sizeof(pending));
  return res.is_error() ? (uint64_t)-14 : 0;
}

}
