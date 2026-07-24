#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Synchronization/spinlock.h>

uint64_t SchedulerManager::process_count() {
  uint64_t count = 0;
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    fk::synchronization::ScopedLockIRQ per_cpu_lock(m_processors[i].run_queue_lock);
    if (m_processors[i].current_task) ++count;
    count += (uint64_t)m_processors[i].run_queue.size();
  }
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  count += (uint64_t)m_wait_queue.size();
  count += (uint64_t)m_sleep_queue.size();
  return count ? count : 1;
}

void SchedulerManager::print_all_tasks() {
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    fk::synchronization::ScopedLockIRQ per_cpu_lock(m_processors[i].run_queue_lock);
    if (m_processors[i].current_task) m_processors[i].current_task->print_info();
    if (m_processors[i].idle_task) m_processors[i].idle_task->print_info();
    for (auto& task : m_processors[i].run_queue) task.print_info();
  }
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  for (auto& task : m_wait_queue) task.print_info();
  for (auto& task : m_zombie_queue) task.print_info();
  for (auto& task : m_sleep_queue) task.print_info();
}

fk::RefPtr<Task> SchedulerManager::find_task(fk::ProcessId id) {
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    fk::synchronization::ScopedLockIRQ per_cpu_lock(m_processors[i].run_queue_lock);
    if (m_processors[i].current_task && m_processors[i].current_task->control.identity.id == id)
      return m_processors[i].current_task;
    if (m_processors[i].idle_task && m_processors[i].idle_task->control.identity.id == id)
      return m_processors[i].idle_task;
    for (auto& task : m_processors[i].run_queue) {
      if (task.control.identity.id == id) return &task;
    }
  }
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  for (auto& task : m_wait_queue) if (task.control.identity.id == id) return &task;
  for (auto& task : m_zombie_queue) if (task.control.identity.id == id) return &task;
  for (auto& task : m_sleep_queue) if (task.control.identity.id == id) return &task;
  if (id.is_valid())
    fk::algorithms::kdebug("SCHEDULER", "find_task: PID %lu not found", id.value());
  return nullptr;
}

fk::RefPtr<Task> SchedulerManager::find_terminated_child(fk::ProcessId ppid) {
  fk::synchronization::ScopedLock lock(m_lock);
  for (auto& task : m_zombie_queue) {
    if (task.control.identity.ppid == ppid && task.control.lifecycle.terminated) return &task;
  }
  return nullptr;
}

void SchedulerManager::send_signal_to_pgrp(int pgid, int signum) {
  if (pgid <= 0) return;
  fk::ProcessId target_pgid((uint64_t)pgid);
  fk::algorithms::kdebug("SCHEDULER", "Sending signal %d to process group %d", signum, pgid);
  auto send = [&](Task& task) {
    if (task.control.identity.pgid == target_pgid)
      fkernel::ipc::SignalDelivery::send_signal(&task, signum);
  };
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    if (m_processors[i].current_task) send(*m_processors[i].current_task);
    fk::synchronization::ScopedLockIRQ per_cpu_lock(m_processors[i].run_queue_lock);
    for (auto& task : m_processors[i].run_queue) send(task);
  }
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  for (auto& task : m_wait_queue)  send(task);
  for (auto& task : m_sleep_queue) send(task);
}

fk::RefPtr<Task> SchedulerManager::find_any_child(fk::ProcessId ppid) {
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    {
      fk::synchronization::ScopedLockIRQ per_cpu_lock(m_processors[i].run_queue_lock);
      if (m_processors[i].current_task && m_processors[i].current_task->control.identity.ppid == ppid)
        return m_processors[i].current_task;
      for (auto& task : m_processors[i].run_queue) if (task.control.identity.ppid == ppid) return &task;
    }
  }
  fk::synchronization::ScopedLock lock(m_lock);
  for (auto& task : m_wait_queue) if (task.control.identity.ppid == ppid) return &task;
  for (auto& task : m_zombie_queue) if (task.control.identity.ppid == ppid) return &task;
  for (auto& task : m_sleep_queue) if (task.control.identity.ppid == ppid) return &task;
  return nullptr;
}
