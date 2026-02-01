#include <Kernel/Ipc/ipc_log_node.h>
#include <Kernel/Ipc/notification.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace ipc {

void Notification::signal(uint64_t bits) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_pending_bits |= bits;

  if (!m_waiting_tasks.is_empty()) {
    Task& task = *m_waiting_tasks.first();
    m_waiting_tasks.remove(task);
    uint32_t task_id = task.control.identity.id.value();

    // Return the bits via the task's context (rax)
    task.registers().rax = m_pending_bits;
    uint64_t delivered_bits = m_pending_bits;
    m_pending_bits = 0;

    IpcLogNode::the()->log_notification_operation("signal_wake", task_id, delivered_bits);
    SchedulerManager::the().wake_task(&task);
  } else {
    // No task waiting, just store bits

    IpcLogNode::the()->log_notification_operation("signal_queue", 0, bits);
  }
}

uint64_t Notification::wait() {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t task_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (m_pending_bits != 0) {
      uint64_t bits = m_pending_bits;
      m_pending_bits = 0;

      IpcLogNode::the()->log_notification_operation("wait_immediate", task_id, bits);
      return bits;
    }

    IpcLogNode::the()->log_notification_operation("wait_blocked", task_id, m_pending_bits);
    m_waiting_tasks.append(*current);
    scheduler.block_current();
  }

  // Result will be set in rax by signal()
  uint64_t result = current->registers().rax;

  IpcLogNode::the()->log_notification_operation("wait_woken", task_id, result);
  return result;
}

uint64_t Notification::poll() {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  uint64_t bits = m_pending_bits;
  m_pending_bits = 0;

  if (bits != 0) {
  }
  IpcLogNode::the()->log_notification_operation("poll", 0, bits);
  return bits;
}

} // namespace ipc
} // namespace fkernel
