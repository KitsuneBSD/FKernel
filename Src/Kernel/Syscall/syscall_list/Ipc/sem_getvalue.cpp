#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/SemFs/sem_dir_node.h>
#include <Kernel/Fs/Virtual/SemFs/sem_node.h>
#include <Kernel/Fs/Vfs/Core/definitions.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

static fk::RefPtr<SemNode> resolve_sem(int fd) {
  auto* task = SchedulerManager::the().current();
  if (!task) return nullptr;
  auto desc = task->get_file_descriptor(fd);
  if (!desc) return nullptr;
  auto node = desc->node();
  if (!node || !node->is_semaphore()) return nullptr;
  return fk::RefPtr<SemNode>(static_cast<SemNode*>(node.get()));
}

// sys_sem_getvalue(...) → 0 or -errno
extern "C" uint64_t sys_sem_getvalue(uint64_t fd, uint64_t val_ptr, uint64_t, uint64_t, uint64_t, uint64_t,
                           [[maybe_unused]] PtRegs* regs) {
  auto sem = resolve_sem((int)fd);
  if (!sem) return (uint64_t)-9;
  int val = sem->get_value();
  if (val_ptr) {
    auto res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(val_ptr), &val, sizeof(int));
    if (res.is_error()) return (uint64_t)-14;
  }
  return 0;
}
