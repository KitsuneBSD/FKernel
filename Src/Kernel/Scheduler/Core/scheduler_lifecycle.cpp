#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Synchronization/interrupt_disabler.h>

#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Fs/Virtual/TimerFd/timer_fd_registry.h>
#include <Kernel/Fs/Vfs/Events/kqueue.h>
#include <Kernel/Net/Sockets/tcp_socket.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Syscall/posix_timer.h>

using namespace fk::synchronization;
using namespace fkernel::scheduler;

extern "C" void fkernel_futex_wake_one(uint64_t uaddr);

void SchedulerManager::block_current() {
  ScopedInterruptDisabler intr_disabler;
  auto& proc = current_processor();
  if (!proc.current_task) return;

  Task* task = proc.current_task;
  if (task->control.lifecycle.in_wait_queue) return;

  task->control.lifecycle.state = TaskState::Blocked;
  fk::algorithms::kdebug("SCHEDULER", "Task %lu blocked", task->control.identity.id.value());
  {
    ScopedLock lock(m_lock);
    task->control.lifecycle.in_wait_queue = true;
    m_wait_queue.push_back(task);
  }
  proc.need_resched = true;
}

void SchedulerManager::block_current_noqueue() {
  ScopedInterruptDisabler intr_disabler;
  auto& proc = current_processor();
  if (!proc.current_task) return;

  Task* task = proc.current_task;
  task->control.lifecycle.state = TaskState::Blocked;
  fk::algorithms::kdebug("SCHEDULER", "Task %lu blocked (ipc)", task->control.identity.id.value());
  proc.need_resched = true;
}

void SchedulerManager::zombify_current() {
  ScopedInterruptDisabler intr_disabler;
  auto& proc = current_processor();
  if (!proc.current_task) return;

  Task* task = proc.current_task;
  task->control.lifecycle.state = TaskState::Zombie;
  task->control.lifecycle.terminated = true;
  fk::algorithms::kdebug("SCHEDULER", "Task %lu zombified", task->control.identity.id.value());
  {
    ScopedLock lock(m_lock);
    m_zombie_queue.push_back(task);
  }
  proc.need_resched = true;
}

void SchedulerManager::sleep_current(fk::TickCount ticks) {
  ScopedInterruptDisabler intr_disabler;
  auto& proc = current_processor();
  if (!proc.current_task) return;

  Task* task = proc.current_task;
  task->control.lifecycle.state = TaskState::Sleeping;
  task->control.lifecycle.wake_up_time_ticks = TickManager::the().get_ticks() + ticks.value();
  fk::algorithms::kdebug("SCHEDULER", "Task %lu sleeping for %lu ticks", task->control.identity.id.value(), ticks.value());
  {
    ScopedLock lock(m_lock);
    // Sorted insertion keeps sleep queue ordered by wakeup time (earliest first).
    // on_tick() can then stop at the first non-ready task instead of scanning all.
    uint64_t wakeup = task->control.lifecycle.wake_up_time_ticks;
    m_sleep_queue.insert_sorted(task, [wakeup](Task* a, Task* b) {
      (void)a;
      return wakeup < b->control.lifecycle.wake_up_time_ticks;
    });
  }
  proc.need_resched = true;
}

[[noreturn]] void SchedulerManager::kill_current_from_exception(int signal) {
  Task* curr = this->current();
  if (!curr) arch_halt_loop();
  // Minimal cleanup safe to call from exception context (no copy_to_user, no file lock walks).
  curr->control.lifecycle.terminated = true;
  curr->control.lifecycle.exit_status = 128 + signal;
  fkernel::notify_proc_kqueue(curr, fkernel::NOTE_EXIT);
  if (curr->control.identity.ppid.is_valid()) {
    auto parent = find_task(curr->control.identity.ppid);
    if (parent) {
      siginfo_t si{};
      si.si_signo = SIGCHLD;
      si.si_code  = 2; // CLD_KILLED
      si.si_pid   = curr->control.identity.id.value();
      si.si_uid   = curr->control.identity.uid;
      si.si_status = signal;
      fkernel::ipc::SignalDelivery::send_signal(parent.get(), SIGCHLD, &si, false);
    }
  }
  zombify_current();
  schedule();
  arch_halt_loop();
}

void SchedulerManager::reap_zombie(Task* task) {
  if (!task || !task->is_valid()) return;
  ScopedInterruptDisabler intr_disabler;
  fk::algorithms::kdebug("SCHEDULER", "Reaping zombie task %lu", task->control.identity.id.value());
  {
    fk::synchronization::ScopedLockIRQ reg_lock(m_task_registry_lock);
    m_task_registry.remove(task->control.identity.id);
  }
  {
    ScopedLock lock(m_lock);
    m_zombie_queue.remove(task);
  }
  task->invalidate();
  task->destroy();
  task->unref();
}

void SchedulerManager::wake_task(Task* task) {
  if (!task || !task->is_valid()) return;
  ScopedInterruptDisabler intr_disabler;
  {
    ScopedLock lock(m_lock);
    TaskState state = task->control.lifecycle.state;
    if (state == TaskState::Ready || state == TaskState::Running)
      return;
    if (state == TaskState::Blocked && task->control.lifecycle.in_wait_queue) {
      m_wait_queue.remove(task);
      task->control.lifecycle.in_wait_queue = false;
    } else if (state == TaskState::Sleeping) {
      m_sleep_queue.remove(task);
    }
  }

  task->control.lifecycle.state = TaskState::Ready;
  task->control.lifecycle.time_slice_ticks = quantum_for_level(fk::MlqfLevel(task->control.lifecycle.mlfq_level)).value();
  fk::algorithms::kdebug("SCHEDULER", "Task %lu woken (level=%d)", task->control.identity.id.value(), task->control.lifecycle.mlfq_level);

  uint32_t target_cpu = 0;
  for (uint32_t i = 0; i < 32; ++i) {
    if (task->control.lifecycle.cpu_affinity & (1ULL << i)) {
      target_cpu = i;
      break;
    }
  }
  if (target_cpu >= m_processor_count.value())
    target_cpu = 0;

  fk::algorithms::ktrace("SCHEDULER", "wake_task pid=%lu -> cpu=%u level=%d quantum=%lu",
                         task->control.identity.id.value(), target_cpu,
                         task->control.lifecycle.mlfq_level,
                         task->control.lifecycle.time_slice_ticks);
  {
    ScopedLock lock(m_processors[target_cpu].run_queue_lock);
    m_processors[target_cpu].run_queues[task->control.lifecycle.mlfq_level].queue.push_back(task);
  }
}

void SchedulerManager::terminate_current(int status) {
  Task* curr = this->current();
  if (!curr) return;

  curr->release_all_file_locks();

  fk::algorithms::klog("SCHEDULER", "Task %lu exiting with status %d", curr->control.identity.id.value(), status);
  // Notify kqueue watchers (EVFILT_PROC NOTE_EXIT) before the task is zombified.
  fkernel::notify_proc_kqueue(curr, fkernel::NOTE_EXIT);
  curr->control.lifecycle.terminated = true;
  curr->control.lifecycle.exit_status = status;

  if (curr->control.lifecycle.clear_child_tid) {
    uintptr_t tid_addr = curr->control.lifecycle.clear_child_tid;
    uint32_t zero = 0;
    fkernel::memory::copy_to_user(reinterpret_cast<void*>(tid_addr), &zero, sizeof(zero));
    fkernel_futex_wake_one(tid_addr);
    curr->control.lifecycle.clear_child_tid = 0;
  }

  if (curr->control.identity.ppid.is_valid()) {
    auto parent = find_task(curr->control.identity.ppid);
    if (parent) {
      if (parent->control.lifecycle.vfork_waiting &&
          curr->control.lifecycle.vfork_parent_id == parent->control.identity.id) {
        parent->control.lifecycle.vfork_waiting = false;
        wake_task(parent.get());
      }

      siginfo_t si{};
      si.si_signo  = SIGCHLD;
      si.si_code   = 1;
      si.si_pid    = curr->control.identity.id.value();
      si.si_uid    = curr->control.identity.uid;
      si.si_status = status;
      // Deliver SIGCHLD to the parent's thread group (not just the spawning thread)
      fkernel::ipc::SignalDelivery::deliver_to_group(
          SIGCHLD, parent->control.identity.tgid, &si);
    }
  }

  zombify_current();
  schedule();
}

static uint32_t find_least_loaded_cpu(fkernel::Processor* processors, uint32_t count) {
  uint32_t best_cpu = 0;
  size_t min_tasks;
  {
    fk::synchronization::ScopedLockIRQ lock(processors[0].run_queue_lock);
    min_tasks = processors[0].run_queue_total_size();
  }
  for (uint32_t i = 1; i < count; ++i) {
    fk::synchronization::ScopedLockIRQ lock(processors[i].run_queue_lock);
    size_t tasks = processors[i].run_queue_total_size();
    if (tasks < min_tasks) {
      min_tasks = tasks;
      best_cpu = i;
    }
  }
  return best_cpu;
}

void SchedulerManager::add_task(Task* task) {
  if (!task || !task->is_valid()) return;
  ScopedInterruptDisabler intr_disabler;

  {
    fk::synchronization::ScopedLockIRQ reg_lock(m_task_registry_lock);
    m_task_registry.insert(task->control.identity.id, task);
  }

  task->control.lifecycle.state = TaskState::Ready;
  task->control.lifecycle.time_slice_ticks = quantum_for_level(fk::MlqfLevel(task->control.lifecycle.mlfq_level)).value();
  fk::algorithms::klog("SCHEDULER", "Task %lu added at MLFQ level %d", task->control.identity.id.value(), task->control.lifecycle.mlfq_level);

  uint32_t target_cpu = find_least_loaded_cpu(&m_processors[0], m_processor_count.value());
  fk::algorithms::ktrace("SCHEDULER", "add_task pid=%lu: target_cpu=%u quantum=%lu",
                         task->control.identity.id.value(), target_cpu,
                         task->control.lifecycle.time_slice_ticks);
  if (task->control.lifecycle.cpu_affinity != 0) {
    for (uint32_t i = 0; i < 32; ++i) {
      if (task->control.lifecycle.cpu_affinity & (1ULL << i)) {
        target_cpu = i;
        break;
      }
    }
    if (target_cpu >= m_processor_count.value()) target_cpu = 0;
  }

  {
    ScopedLock lock(m_processors[target_cpu].run_queue_lock);
    m_processors[target_cpu].run_queues[task->control.lifecycle.mlfq_level].queue.push_back(task);
  }
}

void SchedulerManager::yield() {
  ScopedInterruptDisabler intr_disabler;

  auto& proc = current_processor();
  if (proc.current_task) {
    Task* task = proc.current_task;
    if (task->control.lifecycle.state == TaskState::Running) {
      task->control.lifecycle.state = TaskState::Ready;
      uint8_t level = task->control.lifecycle.mlfq_level;
      {
        ScopedLock lock(proc.run_queue_lock);
        proc.run_queues[level].queue.push_back(task);
      }
      proc.need_resched = true;
      schedule();
    }
  }
}

void SchedulerManager::priority_boost_all() {
  for (uint32_t cpu = 0; cpu < m_processor_count.value(); ++cpu) {
    fk::synchronization::ScopedLockIRQ lock(m_processors[cpu].run_queue_lock);
    for (uint32_t level = 1; level < MLFQ_LEVELS; ++level) {
      while (!m_processors[cpu].run_queues[level].queue.empty()) {
        Task* task = m_processors[cpu].run_queues[level].queue.pop_front();
        task->control.lifecycle.mlfq_level = 0;
        task->control.lifecycle.cpu_time_consumed = 0;
        task->control.lifecycle.time_slice_ticks = quantum_for_level(fk::MlqfLevel(0)).value();
        m_processors[cpu].run_queues[0].queue.push_back(task);
      }
    }
  }
}

void SchedulerManager::on_tick() {
  ScopedInterruptDisabler intr_disabler;
  uint64_t now = TickManager::the().get_ticks();
  auto& proc = current_processor();

  {
    ScopedLock lock(m_lock);
    // Sleep queue is sorted by wakeup time (earliest first).
    // Stop at the first task not yet due — all following tasks sleep longer.
    Task* task;
    while ((task = m_sleep_queue.front()) != nullptr) {
      if (task->control.lifecycle.wake_up_time_ticks > now) break;
      m_sleep_queue.remove(task);
      wake_task(task);
    }
  }

  if (proc.current_task && proc.current_task != proc.idle_task) {
    auto& timer = proc.current_task->control.lifecycle.itimers[0];
    if (timer.active && timer.remaining_ticks > 0) {
      --timer.remaining_ticks;
      if (timer.remaining_ticks == 0) {
        fkernel::ipc::SignalDelivery::send_signal(proc.current_task, timer.signo);
        timer.active = timer.interval_ticks > 0;
        if (timer.interval_ticks > 0)
          timer.remaining_ticks = timer.interval_ticks;
      }
    }
  }

  {
    ScopedLock t_lock(s_timer_lock);
    for (size_t i = 0; i < s_timers.size(); ++i) {
      if (!s_timers[i].used || s_timers[i].expiry_ticks == 0) continue;
      --s_timers[i].expiry_ticks;
      if (s_timers[i].expiry_ticks == 0) {
        Task* owner = s_timers[i].owner;
        if (owner)
          fkernel::ipc::SignalDelivery::send_signal(owner, s_timers[i].signo);
        s_timers[i].used = s_timers[i].interval_ticks > 0;
        if (s_timers[i].interval_ticks > 0)
          s_timers[i].expiry_ticks = s_timers[i].interval_ticks;
      }
    }
  }

  fkernel::timer_fd_registry::tick_all(now, TickManager::the().get_frequency());

  fkernel::net::TcpSocket::tick_all(now);

  m_global_tick_counter = fk::TickCount(m_global_tick_counter.value() + 1);
  if (m_global_tick_counter.value() % BOOST_PERIOD_TICKS == 0) {
    priority_boost_all();
  }

  bool is_run_queue_empty = proc.all_queues_empty();

  if (!proc.current_task || proc.current_task == proc.idle_task || !is_run_queue_empty) {
    proc.need_resched = true;
  } else if (proc.current_task->control.lifecycle.state == TaskState::Running) {
    Task* task = proc.current_task;
    ++task->control.lifecycle.cpu_time_consumed;

    if (task->control.lifecycle.policy == SchedulingPolicy::Fifo)
      return;

    if (--task->control.lifecycle.time_slice_ticks == 0) {
      if (task->control.lifecycle.policy == SchedulingPolicy::RoundRobin) {
        task->control.lifecycle.state = TaskState::Ready;
        uint8_t level = task->control.lifecycle.mlfq_level;
        {
          ScopedLock lock(proc.run_queue_lock);
          proc.run_queues[level].queue.push_back(task);
        }
        proc.need_resched = true;
        return;
      }

      if (task->control.lifecycle.mlfq_level < MLFQ_LEVELS - 1 &&
          task->control.lifecycle.cpu_time_consumed >= task->control.lifecycle.allotment_ticks) {
        ++task->control.lifecycle.mlfq_level;
        task->control.lifecycle.cpu_time_consumed = 0;
        fk::algorithms::ktrace("SCHEDULER", "pid=%lu demoted to MLFQ level %d",
                               task->control.identity.id.value(), task->control.lifecycle.mlfq_level);
      }

      task->control.lifecycle.state = TaskState::Ready;
      uint8_t new_level = task->control.lifecycle.mlfq_level;
      task->control.lifecycle.time_slice_ticks = quantum_for_level(fk::MlqfLevel(new_level)).value();
      {
        ScopedLock lock(proc.run_queue_lock);
        proc.run_queues[new_level].queue.push_back(task);
      }
      proc.need_resched = true;
    }
  }
}
