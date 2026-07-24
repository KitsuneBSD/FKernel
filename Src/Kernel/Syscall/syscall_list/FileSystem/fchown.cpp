#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" uint64_t sys_fchown(uint64_t fd, uint64_t uid, uint64_t gid,
                               uint64_t, uint64_t, uint64_t,
                               [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;
  auto desc = task->get_file_descriptor((int)fd);
  if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
  auto node = desc->node();
  if (!node) return fkernel::return_error(fk::core::Error::InvalidHandle);
  node->set_owner((uint32_t)uid, (uint32_t)gid);
  return 0;
}
