#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" uint64_t sys_fchmod(uint64_t fd, uint64_t mode,
                               uint64_t, uint64_t, uint64_t, uint64_t,
                               [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;
  auto desc = task->get_file_descriptor((int)fd);
  if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
  auto node = desc->node();
  if (!node) return fkernel::return_error(fk::core::Error::InvalidHandle);
  node->set_permissions((uint32_t)mode);
  return 0;
}
