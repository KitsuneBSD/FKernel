#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
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
