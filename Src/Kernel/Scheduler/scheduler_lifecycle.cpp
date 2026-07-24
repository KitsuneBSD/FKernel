#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Fs/TimerFd/timer_fd_registry.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Synchronization/interrupt_disabler.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>

using namespace fk::synchronization;

extern "C" void fkernel_futex_wake_one(uintptr_t uaddr);

// POSIX timer globals from timer_posix.cpp
struct PosixTimerEntry { bool used; int signo; uint64_t interval_ticks;
  uint64_t expiry_ticks; Task* owner; };
extern PosixTimerEntry s_timers[];
static constexpr int POSIX_TIMER_MAX = 8;

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

void SchedulerManager::sleep_current(uint64_t sleep_ticks) {
  ScopedInterruptDisabler intr_disabler;
  auto& proc = current_processor();
  if (!proc.current_task) return;

  Task* task = proc.current_task;
  task->control.lifecycle.state = TaskState::Sleeping;
  task->control.lifecycle.wake_up_time_ticks = TickManager::the().get_ticks() + sleep_ticks;
  fk::algorithms::kdebug("SCHEDULER", "Task %lu sleeping for %lu ticks", task->control.identity.id.value(), sleep_ticks);
  {
    ScopedLock lock(m_lock);
    m_sleep_queue.push_back(task);
  }
  proc.need_resched = true;
}

void SchedulerManager::reap_zombie(Task* task) {
  if (!task || !task->is_valid()) return;
  ScopedInterruptDisabler intr_disabler;
  fk::algorithms::kdebug("SCHEDULER", "Reaping zombie task %lu", task->control.identity.id.value());
  {
    ScopedLock lock(m_lock);
    m_zombie_queue.remove(task);
  }
  task->invalidate();
  task->destroy();
  task->unref(); // drop scheduler's reference; deletes if no RefPtr holders remain
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
  task->control.lifecycle.time_slice_ticks = m_default_quantum;
  fk::algorithms::kdebug("SCHEDULER", "Task %lu woken", task->control.identity.id.value());

  uint32_t target_cpu = 0;
  for (uint32_t i = 0; i < 32; ++i) {
    if (task->control.lifecycle.cpu_affinity & (1ULL << i)) {
      target_cpu = i;
      break;
    }
  }
  if (target_cpu >= m_processor_count)
    target_cpu = 0;

  {
    ScopedLock lock(m_processors[target_cpu].run_queue_lock);
    m_processors[target_cpu].run_queue.push_back(task);
  }
}

void SchedulerManager::terminate_current(int status) {
  Task* curr = this->current();
  if (!curr) return;

  fk::algorithms::klog("SCHEDULER", "Task %lu exiting with status %d", curr->control.identity.id.value(), status);
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
      }
      fkernel::ipc::SignalDelivery::send_signal(parent.get(), SIGCHLD);
      wake_task(parent.get());
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
    min_tasks = processors[0].run_queue.size();
  }
  for (uint32_t i = 1; i < count; ++i) {
    fk::synchronization::ScopedLockIRQ lock(processors[i].run_queue_lock);
    size_t tasks = processors[i].run_queue.size();
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

  task->control.lifecycle.state = TaskState::Ready;
  task->control.lifecycle.time_slice_ticks = m_default_quantum;
  fk::algorithms::klog("SCHEDULER", "Task %lu added to run queue", task->control.identity.id.value());

  uint32_t target_cpu = find_least_loaded_cpu(m_processors, m_processor_count);
  if (task->control.lifecycle.cpu_affinity != 0) {
    for (uint32_t i = 0; i < 32; ++i) {
      if (task->control.lifecycle.cpu_affinity & (1ULL << i)) {
        target_cpu = i;
        break;
      }
    }
    if (target_cpu >= m_processor_count) target_cpu = 0;
  }

  {
    ScopedLock lock(m_processors[target_cpu].run_queue_lock);
    m_processors[target_cpu].run_queue.push_back(task);
  }
}

void SchedulerManager::yield() {
  ScopedInterruptDisabler intr_disabler;

  auto& proc = current_processor();
  if (proc.current_task) {
    Task* task = proc.current_task;
    if (task->control.lifecycle.state == TaskState::Running) {
      task->control.lifecycle.state = TaskState::Ready;
      {
        ScopedLock lock(proc.run_queue_lock);
        proc.run_queue.push_back(task);
      }
      proc.need_resched = true;
      schedule();
    }
  }
}

void SchedulerManager::on_tick() {
  ScopedInterruptDisabler intr_disabler;
  uint64_t now = TickManager::the().get_ticks();
  auto& proc = current_processor();

  {
    ScopedLock lock(m_lock);
    for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end();) {
      Task* task = &*it;
      ++it;
      if (task->control.lifecycle.wake_up_time_ticks <= now) {
        m_sleep_queue.remove(task);
        wake_task(task);
      }
    }
  }

  // Process ITIMER_REAL for the current task
  if (proc.current_task && proc.current_task != proc.idle_task) {
    auto& timer = proc.current_task->control.lifecycle.itimers[0]; // ITIMER_REAL
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

  // Process POSIX timers (global, deliver to owner task)
  for (int i = 0; i < POSIX_TIMER_MAX; ++i) {
    if (!s_timers[i].used || s_timers[i].expiry_ticks == 0) continue;
    --s_timers[i].expiry_ticks;
    if (s_timers[i].expiry_ticks == 0) {
      Task* owner = s_timers[i].owner;
      if (owner) {
        fkernel::ipc::SignalDelivery::send_signal(owner, s_timers[i].signo);
      }
      s_timers[i].used = s_timers[i].interval_ticks > 0;
      if (s_timers[i].interval_ticks > 0)
        s_timers[i].expiry_ticks = s_timers[i].interval_ticks;
    }
  }

  fkernel::timer_fd_registry::tick_all(now, TickManager::the().get_frequency());

  bool is_run_queue_empty = false;
  {
    ScopedLock lock(proc.run_queue_lock);
    is_run_queue_empty = proc.run_queue.empty();
  }

  if (!proc.current_task || proc.current_task == proc.idle_task || !is_run_queue_empty) {
    proc.need_resched = true;
  } else if (proc.current_task->control.lifecycle.state == TaskState::Running &&
             --proc.current_task->control.lifecycle.time_slice_ticks == 0) {
    Task* task = proc.current_task;
    task->control.lifecycle.state = TaskState::Ready;
    {
      ScopedLock lock(proc.run_queue_lock);
      proc.run_queue.push_back(task);
    }
    proc.need_resched = true;
  }
}