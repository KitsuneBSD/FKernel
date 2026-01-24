#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uintptr_t sys_wait4(uintptr_t pid_val, uintptr_t status_ptr, uintptr_t options,
                    [[maybe_unused]] uintptr_t rusage_ptr, uintptr_t, uintptr_t) {
  int64_t pid = (int64_t)pid_val;
  auto* current_task = SchedulerManager::the().current();

  fk::algorithms::klog("SYSCALL", "wait4: PID %lu waiting for child PID %ld (options=%lu)", current_task->id, pid, options);

  while (true) {
      Task* task = nullptr;
      if (pid == -1) {
          task = SchedulerManager::the().find_terminated_child(current_task->id);
      } else {
          task = SchedulerManager::the().find_task(pid);
          if (task && (!task->terminated || task->ppid != current_task->id)) {
              task = nullptr;
          }
      }

      if (task) {
          if (status_ptr) {
              int* status = reinterpret_cast<int*>(status_ptr);
              // Shift the exit status to match standard Linux/POSIX wait status format
              // where the low 8 bits of status are the exit code shifted by 8.
              // Actually, simplified: just store it.
              *status = (task->exit_status << 8); 
          }
          
          uint64_t child_id = task->id;
          fk::algorithms::klog("SYSCALL", "wait4: Reaping zombie PID %lu", child_id);
          
          // Simple reap: let's not worry about memory for now, but mark it
          task->ppid = 0; 
          return child_id;
      }

      // No zombie found. Should we block?
      // Check if ANY child exists
      bool has_children = false;
      if (pid == -1) {
          has_children = SchedulerManager::the().find_any_child(current_task->id) != nullptr;
      } else {
          Task* t = SchedulerManager::the().find_task(pid);
          has_children = (t && t->ppid == current_task->id);
      }

      if (!has_children) {
          return fkernel::return_error(fk::core::Error::NoChildProcesses);
      }

      if (options & 1) { // WNOHANG (assuming 1 is WNOHANG)
          return 0;
      }

      // Block until a child wakes us up (sys_exit does this)
      fk::algorithms::klog("SYSCALL", "wait4: No zombie yet, blocking PID %lu", current_task->id);
      SchedulerManager::the().block_current();
      SchedulerManager::the().schedule(); // Trigger reschedule and switch context
  }
}
}
