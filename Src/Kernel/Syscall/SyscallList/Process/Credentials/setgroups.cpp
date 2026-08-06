#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {

uint64_t sys_setgroups(uint64_t size, uint64_t list_ptr, uint64_t,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return (uint64_t)-1;
  if (t->control.identity.euid != 0)
    return fkernel::return_error(fk::core::Error::PermissionDenied);
  static constexpr uint32_t MAX_GROUPS = 16;
  if (size > MAX_GROUPS)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  if (size == 0) {
    t->control.identity.ngroups = 0;
    return 0;
  }
  if (!list_ptr)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  fk::memory::copy(t->control.identity.supplementary_gids,
         reinterpret_cast<const uint32_t*>(list_ptr),
         size * sizeof(uint32_t));
  t->control.identity.ngroups = (uint32_t)size;
  return 0;
}

}
