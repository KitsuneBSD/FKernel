#include <Kernel/Ipc/notification.h>
#include <Kernel/Scheduler/scheduler.h>

namespace fkernel {
namespace ipc {

void Notification::signal(uint64_t bits) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_pending_bits |= bits;

  if (!m_waiting_tasks.is_empty()) {
    Task &task = *m_waiting_tasks.first();
    m_waiting_tasks.remove(task);

    // Return the bits via the task's context (rax)
    task.registers().rax = m_pending_bits;
    m_pending_bits = 0;

    SchedulerManager::the().wake_task(&task);
  }
}

uint64_t Notification::wait() {
  auto &scheduler = SchedulerManager::the();
  Task *current = scheduler.current();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (m_pending_bits != 0) {
      uint64_t bits = m_pending_bits;
      m_pending_bits = 0;
      return bits;
    }

    m_waiting_tasks.append(*current);
    scheduler.block_current();
  }

  // Result will be set in rax by signal()
  return current->registers().rax;
}

uint64_t Notification::poll() {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  uint64_t bits = m_pending_bits;
  m_pending_bits = 0;
  return bits;
}

} // namespace ipc
} // namespace fkernel
