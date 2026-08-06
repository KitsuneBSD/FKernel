#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/error.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {

uint64_t sys_wait4(uint64_t pid_val, uint64_t status_ptr, uint64_t options,
                   [[maybe_unused]] uint64_t rusage_ptr, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
  fk::ProcessId pid = fk::ProcessId::from_signed(static_cast<int64_t>(pid_val));
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  while (true) {
    fk::RefPtr<Task> task;
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
      if (!task->is_valid()) {
        fk::algorithms::kwarn("WAIT", "Task %lu has invalid magic, skipping",
                              pid.value());
        return fkernel::return_error(fk::core::Error::InvalidHandle);
      }

      if (status_ptr) {
        int encoded = task->control.lifecycle.killed_by_signal
            ? (task->control.lifecycle.exit_status & 0x7f)
            : (task->control.lifecycle.exit_status << 8);
        fkernel::memory::copy_to_user(reinterpret_cast<void*>(status_ptr), &encoded, sizeof(int));
      }

      uint64_t child_id = task->control.identity.id.value();
      int exit_status_val = task->control.lifecycle.exit_status;

      SchedulerManager::the().reap_zombie(task.get());
      fk::algorithms::kdebug("WAIT", "wait4: reaped child %lu, status=%d", child_id,
                             exit_status_val);
      return child_id;
    }

    bool has_children = false;
    if (pid.is_any()) {
      has_children = !!SchedulerManager::the().find_any_child(current_task->control.identity.id);
    } else {
      auto t = SchedulerManager::the().find_task(pid);
      has_children = (t && t->control.identity.ppid == current_task->control.identity.id);
    }

    if (!has_children) {
      fk::algorithms::kdebug("WAIT", "wait4: no children");
      return fkernel::return_error(fk::core::Error::NoChildProcesses);
    }

    if (options & 1) // WNOHANG
      return 0;

    SchedulerManager::the().block_current();
    SchedulerManager::the().schedule();

    // After waking up (e.g. SIGCHLD), check if the waited child has now exited
    // BEFORE checking pending signals — otherwise SIGCHLD causes spurious EINTR.
    {
      fk::RefPtr<Task> zombie;
      if (pid.is_any()) {
        zombie = SchedulerManager::the().find_terminated_child(current_task->control.identity.id);
      } else {
        auto t = SchedulerManager::the().find_task(pid);
        if (t && t->control.lifecycle.terminated &&
            t->control.identity.ppid == current_task->control.identity.id)
          zombie = t;
      }

      if (zombie) {
        if (!zombie->is_valid()) {
          fk::algorithms::kwarn("WAIT", "Zombie task has invalid magic, skipping");
          return fkernel::return_error(fk::core::Error::InvalidHandle);
        }

        if (status_ptr) {
          int encoded = zombie->control.lifecycle.killed_by_signal
              ? (zombie->control.lifecycle.exit_status & 0x7f)
              : (zombie->control.lifecycle.exit_status << 8);
          fkernel::memory::copy_to_user(reinterpret_cast<void*>(status_ptr), &encoded,
                                        sizeof(int));
        }
        uint64_t child_id = zombie->control.identity.id.value();
        int exit_status_val = zombie->control.lifecycle.exit_status;
        SchedulerManager::the().reap_zombie(zombie.get());
        fk::algorithms::kdebug("WAIT", "wait4: reaped child %lu (post-sleep), status=%d",
                               child_id, exit_status_val);
        return child_id;
      }
    }

    if (current_task->has_pending_signals())
      return (uint64_t)(-4); // EINTR
  }
}
}
