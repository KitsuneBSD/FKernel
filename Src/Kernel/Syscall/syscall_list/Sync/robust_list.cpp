#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

// SYS_SET_ROBUST_LIST = 273 — musl pthread_mutex uses robust futex.
// Store the list head pointer in the task so futex cleanup can walk it on exit.
extern "C" uint64_t sys_set_robust_list(uint64_t head, uint64_t len,
                                         uint64_t, uint64_t, uint64_t, uint64_t,
                                         [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;
  if (len != 24) return (uint64_t)-22; // struct robust_list_head is 24 bytes on x86-64
  task->resources.ipc.robust_list_head = head;
  return 0;
}

extern "C" uint64_t sys_get_robust_list([[maybe_unused]] uint64_t pid,
                                         uint64_t head_ptr, uint64_t len_ptr,
                                         uint64_t, uint64_t, uint64_t,
                                         [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  if (head_ptr) {
    uint64_t head = task->resources.ipc.robust_list_head;
    if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(head_ptr),
                                       &head, sizeof(head)).is_error())
      return (uint64_t)-14;
  }
  if (len_ptr) {
    uint64_t sz = 24;
    if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(len_ptr),
                                       &sz, sizeof(sz)).is_error())
      return (uint64_t)-14;
  }
  return 0;
}
