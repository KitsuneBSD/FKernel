#include <Kernel/Ipc/notification.h>
#include <Kernel/Scheduler/scheduler.h>

namespace fkernel {
namespace ipc {

void Notification::signal(uint64_t bits) {
  m_pending_bits |= bits;

  if (!m_waiting_tasks.is_empty()) {
    Task &task = *m_waiting_tasks.first();
    m_waiting_tasks.remove(task);

    // Return the bits via the task's context (rax)
    task.context.rax = m_pending_bits;
    m_pending_bits = 0;

    SchedulerManager::the().wake_task(&task);
  }
}

uint64_t Notification::wait() {
  if (m_pending_bits != 0) {
    uint64_t bits = m_pending_bits;
    m_pending_bits = 0;
    return bits;
  }

  auto &scheduler = SchedulerManager::the();
  Task *current = scheduler.current();

  m_waiting_tasks.append(*current);
  scheduler.block_current();

  // Result will be set in rax by signal()
  return current->context.rax;
}

uint64_t Notification::poll() {
  uint64_t bits = m_pending_bits;
  m_pending_bits = 0;
  return bits;
}

} // namespace ipc
} // namespace fkernel
