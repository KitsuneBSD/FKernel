#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

extern "C" {

uint64_t sys_getgroups(uint64_t size, uint64_t list_ptr, uint64_t,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* t = SchedulerManager::the().current();
  if (!t) return 0;
  uint32_t n = t->control.identity.ngroups;
  if (size == 0 || list_ptr == 0)
    return (uint64_t)n;
  if ((uint64_t)n > size)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  auto* dst = reinterpret_cast<uint32_t*>(list_ptr);
  fk::memory::copy(dst, t->control.identity.supplementary_gids, n * sizeof(uint32_t));
  return (uint64_t)n;
}

}
