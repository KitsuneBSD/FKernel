#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Synchronization/interrupt_disabler.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Driver/Vga/display.h>

using namespace fk::synchronization;

void SchedulerManager::block_current() {
  ScopedInterruptDisabler intr_disabler;
  auto& proc = current_processor();
  if (!proc.current_task) return;

  Task* task = proc.current_task;
  task->control.lifecycle.state = TaskState::Blocked;
  {
    ScopedLock lock(m_lock);
    m_wait_queue.push_back(task);
  }
  proc.need_resched = true;
}

void SchedulerManager::zombify_current() {
  ScopedInterruptDisabler intr_disabler;
  auto& proc = current_processor();
  if (!proc.current_task) return;

  Task* task = proc.current_task;
  task->control.lifecycle.state = TaskState::Blocked;
  task->control.lifecycle.terminated = true;
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
  {
    ScopedLock lock(m_lock);
    m_sleep_queue.push_back(task);
  }
  proc.need_resched = true;
}

void SchedulerManager::reap_zombie(Task* task) {
  if (!task || !task->is_valid()) return;
  ScopedInterruptDisabler intr_disabler;
  {
    ScopedLock lock(m_lock);
    m_zombie_queue.remove(task);
  }
  task->invalidate();
}

void SchedulerManager::wake_task(Task* task) {
  if (!task || !task->is_valid()) return;
  ScopedInterruptDisabler intr_disabler;
  {
    ScopedLock lock(m_lock);
    if (task->control.lifecycle.state == TaskState::Blocked) {
      m_wait_queue.remove(task);
    }
  }

  task->control.lifecycle.state = TaskState::Ready;
  task->control.lifecycle.time_slice_ticks = m_default_quantum;

  uint32_t target_cpu = 0;
  for (uint32_t i = 0; i < 32; ++i) {
    if (task->control.lifecycle.cpu_affinity & (1ULL << i)) {
      target_cpu = i;
      break;
    }
  }

  {
    ScopedLock lock(m_processors[target_cpu].run_queue_lock);
    m_processors[target_cpu].run_queue.push_back(task);
  }
}

void SchedulerManager::terminate_current(int status) {
  Task* curr = this->current();
  if (!curr) return;

  curr->control.lifecycle.terminated = true;
  curr->control.lifecycle.exit_status = status;

  auto* parent = find_task(curr->control.identity.ppid);
  if (parent) {
    if (parent->control.lifecycle.vfork_waiting &&
        curr->control.lifecycle.vfork_parent_id == parent->control.identity.id) {
      parent->control.lifecycle.vfork_waiting = false;
    }
    wake_task(parent);
  }

  zombify_current();
  schedule();
}

void SchedulerManager::add_task(Task* task) {
  if (!task || !task->is_valid()) return;
  ScopedInterruptDisabler intr_disabler;

  task->control.lifecycle.state = TaskState::Ready;
  task->control.lifecycle.time_slice_ticks = m_default_quantum;

  uint32_t target_cpu = 0;
  if (task->control.lifecycle.cpu_affinity != 0) {
    for (uint32_t i = 0; i < 32; ++i) {
      if (task->control.lifecycle.cpu_affinity & (1ULL << i)) {
        target_cpu = i;
        break;
      }
    }
  } else {
    target_cpu = APIC::the().get_id();
  }

  if (target_cpu >= 32) target_cpu = 0;

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

  if (proc.id == 0) {
    Display::the().background_flush();
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

  bool is_run_queue_empty = false;
  {
    ScopedLock lock(proc.run_queue_lock);
    is_run_queue_empty = proc.run_queue.empty();
  }

  if (!proc.current_task || proc.current_task == proc.idle_task || !is_run_queue_empty) {
    proc.need_resched = true;
  } else if (--proc.current_task->control.lifecycle.time_slice_ticks == 0) {
    Task* task = proc.current_task;
    task->control.lifecycle.state = TaskState::Ready;
    {
      ScopedLock lock(proc.run_queue_lock);
      proc.run_queue.push_back(task);
    }
    proc.need_resched = true;
  }
}