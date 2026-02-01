#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_wait4(uint64_t pid_val, uint64_t status_ptr, uint64_t options,
                   [[maybe_unused]] uint64_t rusage_ptr, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
  fk::ProcessId pid = fk::ProcessId::from_signed(static_cast<int64_t>(pid_val));
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  while (true) {
    Task* task = nullptr;
    if (pid.is_any()) {
      task = SchedulerManager::the().find_terminated_child(current_task->control.identity.id);
    } else {
      task = SchedulerManager::the().find_task(pid);
      if (task && (!task->control.lifecycle.terminated ||
                   task->control.identity.ppid != current_task->control.identity.id)) {
        task = nullptr;
      }
    }

    if (task) {
      ASSERT(task->is_valid());
      if (status_ptr) {
        int* status = reinterpret_cast<int*>(status_ptr);
        *status = (task->control.lifecycle.exit_status << 8);
      }

      uint64_t child_id = task->control.identity.id.value();

      SchedulerManager::the().reap_zombie(task);
      return child_id;
    }

    bool has_children = false;
    if (pid.is_any()) {
      has_children =
          SchedulerManager::the().find_any_child(current_task->control.identity.id) != nullptr;
    } else {
      Task* t = SchedulerManager::the().find_task(pid);
      has_children = (t && t->control.identity.ppid == current_task->control.identity.id);
    }

    if (!has_children) {
      return fkernel::return_error(fk::core::Error::NoChildProcesses);
    }

    if (options & 1) { // WNOHANG
      return 0;
    }

    SchedulerManager::the().block_current();
    SchedulerManager::the().schedule();
  }
}
}