#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_wait4(uint64_t pid_val, uint64_t status_ptr, uint64_t options,
                    [[maybe_unused]] uint64_t rusage_ptr, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int64_t pid = (int64_t)pid_val;
  auto* current_task = SchedulerManager::the().current();
  if (!current_task) return -1;

  fk::algorithms::klog("SYSCALL", "wait4: PID %lu waiting for child PID %ld (options=%lu)", 
                       current_task->identity.id.value(), pid, options);

  while (true) {
      Task* task = nullptr;
      if (pid == -1) {
          task = SchedulerManager::the().find_terminated_child(current_task->identity.id);
      } else {
          task = SchedulerManager::the().find_task(fk::ProcessId(pid));
          if (task && (!task->terminated || task->identity.ppid != current_task->identity.id)) {
              task = nullptr;
          }
      }

      if (task) {
          if (status_ptr) {
              int* status = reinterpret_cast<int*>(status_ptr);
              *status = (task->exit_status << 8); 
          }
          
          uint64_t child_id = task->identity.id.value();
          fk::algorithms::klog("SYSCALL", "wait4: Reaping zombie PID %lu", child_id);
          
          task->identity.ppid = fk::ProcessId(); 
          return child_id;
      }

      // No zombie found. Should we block?
      bool has_children = false;
      if (pid == -1) {
          has_children = SchedulerManager::the().find_any_child(current_task->identity.id) != nullptr;
      } else {
          Task* t = SchedulerManager::the().find_task(fk::ProcessId(pid));
          has_children = (t && t->identity.ppid == current_task->identity.id);
      }

      if (!has_children) {
          return fkernel::return_error(fk::core::Error::NoChildProcesses);
      }

      if (options & 1) { // WNOHANG
          fk::algorithms::klog("SYSCALL", "wait4: WNOHANG returning 0");
          return 0;
      }

      fk::algorithms::klog("SYSCALL", "wait4: No zombie yet, blocking PID %lu", 
                           current_task->identity.id.value());
      SchedulerManager::the().block_current();
      SchedulerManager::the().schedule(); 
  }
}
}
