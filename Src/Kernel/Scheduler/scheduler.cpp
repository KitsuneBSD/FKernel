#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Segments/Tss/tss_stacks.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

void SchedulerManager::initialize() {
  m_is_initialized = true;

  fk::algorithms::klog("SCHEDULER MANAGER",
                       "Initializing Scheduler Manager (round robin)...");
}

void SchedulerManager::add_task(Task *task) {
  task->state = TaskState::Ready;
  task->time_slice_ticks = m_default_quantum;
  m_run_queue.push_back(task);
}

void SchedulerManager::block_current() {
  if (!m_current)
    return;

  Task *task = m_current;
  task->state = TaskState::Blocked;
  m_current = nullptr;
}

void SchedulerManager::sleep_current(uint64_t sleep_ticks) {
  if (!m_current)
    return;

  Task *task = m_current;
  task->state = TaskState::Sleeping;
  task->wake_up_time_ticks = TickManager::the().get_ticks() + sleep_ticks;

  m_sleep_queue.push_back(task);
  m_current = nullptr;
}

void SchedulerManager::wake_task(Task *task) {
  task->state = TaskState::Ready;
  task->time_slice_ticks = m_default_quantum;
  m_run_queue.push_back(task);
}

Task *SchedulerManager::pick_next() {
  if (m_run_queue.empty())
    return nullptr;

  Task *next = m_run_queue.front();
  m_run_queue.pop_front();

  next->state = TaskState::Running;
  next->time_slice_ticks = m_default_quantum;
  m_current = next;

  return next;
}

void SchedulerManager::on_tick() {
  uint64_t now = TickManager::the().get_ticks();

  for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end();) {
    Task *task = &*it;
    ++it;

    if (task->wake_up_time_ticks <= now) {
      m_sleep_queue.remove(task);
      wake_task(task);
    }
  }

  if (!m_current)
    return;

  if (--m_current->time_slice_ticks == 0) {
    Task *task = m_current;
    task->state = TaskState::Ready;
    m_run_queue.push_back(task);
    m_current = nullptr;

    is_need_a_resched = true;
  }
}
