#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Synchronization/spinlock.h>

void SchedulerManager::print_all_tasks() {
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    if (m_processors[i].current_task) m_processors[i].current_task->print_info();
    if (m_processors[i].idle_task) m_processors[i].idle_task->print_info();
    for (auto& task : m_processors[i].run_queue) task.print_info();
  }
  for (auto& task : m_wait_queue) task.print_info();
  for (auto& task : m_zombie_queue) task.print_info();
  for (auto& task : m_sleep_queue) task.print_info();
}

Task* SchedulerManager::find_task(fk::ProcessId id) {
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
  return nullptr;
}

Task* SchedulerManager::find_terminated_child(fk::ProcessId ppid) {
  fk::synchronization::ScopedLock lock(m_lock);
  for (auto& task : m_zombie_queue) {
    if (task.control.identity.ppid == ppid && task.control.lifecycle.terminated) return &task;
  }
  return nullptr;
}

Task* SchedulerManager::find_any_child(fk::ProcessId ppid) {
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    if (m_processors[i].current_task && m_processors[i].current_task->control.identity.ppid == ppid) return m_processors[i].current_task;
    {
      fk::synchronization::ScopedLock lock(m_processors[i].run_queue_lock);
      for (auto& task : m_processors[i].run_queue) if (task.control.identity.ppid == ppid) return &task;
    }
  }
  fk::synchronization::ScopedLock lock(m_lock);
  for (auto& task : m_wait_queue) if (task.control.identity.ppid == ppid) return &task;
  for (auto& task : m_zombie_queue) if (task.control.identity.ppid == ppid) return &task;
  for (auto& task : m_sleep_queue) if (task.control.identity.ppid == ppid) return &task;
  return nullptr;
}
