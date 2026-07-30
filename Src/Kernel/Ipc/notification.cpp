#include <Kernel/Ipc/ipc_log_node.h>
#include <Kernel/Ipc/notification.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {
namespace ipc {

bool Notification::payload_pop(NotificationPayload& out) {
  if (m_payload_count == 0) return false;
  out = m_payloads[m_payload_head];
  m_payload_head = (m_payload_head + 1) % NOTIFICATION_MAX_PAYLOADS;
  --m_payload_count;
  return true;
}

void Notification::payload_push(const NotificationPayload& p) {
  if (m_payload_count >= NOTIFICATION_MAX_PAYLOADS) {
    fk::algorithms::kwarn("NOTIFICATION", "Payload queue full, dropping oldest");
    m_payload_head = (m_payload_head + 1) % NOTIFICATION_MAX_PAYLOADS;
    --m_payload_count;
  }
  m_payloads[m_payload_tail] = p;
  m_payload_tail = (m_payload_tail + 1) % NOTIFICATION_MAX_PAYLOADS;
  ++m_payload_count;
}

void Notification::signal(fk::NotificationBits bits) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_pending_bits |= bits;

  if (!m_waiting_tasks.is_empty()) {
    Task& task = *m_waiting_tasks.first();
    m_waiting_tasks.remove(task);
    uint32_t task_id = task.control.identity.id.value();

    task.registers().rax = m_pending_bits.value();
    uint64_t delivered_bits = m_pending_bits.value();
    m_pending_bits.clear_all();

    IpcLogNode::the()->log_notification_operation("signal_wake", task_id, delivered_bits);
    SchedulerManager::the().wake_task(&task);
  } else {
    IpcLogNode::the()->log_notification_operation("signal_queue", 0, bits.value());
  }
}

void Notification::signal_with_payload(fk::NotificationBits bits, const void* data, size_t len) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_pending_bits |= bits;

  NotificationPayload payload;
  payload.bits = bits;
  size_t copy_len = (len < NOTIFICATION_PAYLOAD_SIZE) ? len : NOTIFICATION_PAYLOAD_SIZE;
  if (data && copy_len > 0)
    fk::memory::copy(payload.data, data, copy_len);
  if (copy_len < NOTIFICATION_PAYLOAD_SIZE)
    fk::memory::set(payload.data + copy_len, 0, NOTIFICATION_PAYLOAD_SIZE - copy_len);
  payload_push(payload);

  if (!m_waiting_tasks.is_empty()) {
    Task& task = *m_waiting_tasks.first();
    m_waiting_tasks.remove(task);
    uint32_t task_id = task.control.identity.id.value();

    task.registers().rax = m_pending_bits.value();
    uint64_t delivered_bits = m_pending_bits.value();
    m_pending_bits.clear_all();

    IpcLogNode::the()->log_notification_operation("signal_wake_payload", task_id, delivered_bits);
    SchedulerManager::the().wake_task(&task);
  } else {
    IpcLogNode::the()->log_notification_operation("signal_queue_payload", 0, bits.value());
  }
}

fk::NotificationBits Notification::wait() {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t task_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (!m_pending_bits.is_empty()) {
      fk::NotificationBits bits = m_pending_bits;
      m_pending_bits.clear_all();

      IpcLogNode::the()->log_notification_operation("wait_immediate", task_id, bits.value());
      return bits;
    }

    IpcLogNode::the()->log_notification_operation("wait_blocked", task_id, m_pending_bits.value());
    m_waiting_tasks.append(*current);
    scheduler.block_current_noqueue();
  }

  uint64_t result = current->registers().rax;

  IpcLogNode::the()->log_notification_operation("wait_woken", task_id, result);
  return fk::NotificationBits(result);
}

fk::NotificationBits Notification::wait_timeout(fk::TickCount timeout_ticks) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t task_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (!m_pending_bits.is_empty()) {
      fk::NotificationBits bits = m_pending_bits;
      m_pending_bits.clear_all();

      IpcLogNode::the()->log_notification_operation("wait_timeout_immediate", task_id, bits.value());
      return bits;
    }

    IpcLogNode::the()->log_notification_operation("wait_timeout_blocked", task_id, timeout_ticks.value());
    m_waiting_tasks.append(*current);
  }

  scheduler.sleep_current(timeout_ticks);

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    bool still_waiting = current->wait_node.prev != nullptr
                      || current->wait_node.next != nullptr
                      || m_waiting_tasks.first() == current;
    if (still_waiting) {
      m_waiting_tasks.remove(*current);
      IpcLogNode::the()->log_notification_operation("wait_timeout_expired", task_id, 0);
      return fk::NotificationBits(0);
    }
  }

  uint64_t result = current->registers().rax;
  IpcLogNode::the()->log_notification_operation("wait_timeout_woken", task_id, result);
  return fk::NotificationBits(result);
}

fk::core::Result<fk::NotificationBits, fk::core::Error> Notification::wait_interruptible() {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();

  if (current && current->has_pending_signals())
    return fk::core::Error::Interrupted;

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (current && current->has_pending_signals())
      return fk::core::Error::Interrupted;

    if (!m_pending_bits.is_empty()) {
      fk::NotificationBits bits = m_pending_bits;
      m_pending_bits.clear_all();
      return bits;
    }

    if (current && current->has_pending_signals())
      return fk::core::Error::Interrupted;

    m_waiting_tasks.append(*current);
    scheduler.block_current_noqueue();
  }

  if (current && current->has_pending_signals())
    return fk::core::Error::Interrupted;

  uint64_t result = current->registers().rax;
  return fk::NotificationBits(result);
}

fk::core::Result<fk::NotificationBits, fk::core::Error> Notification::wait_interruptible_timeout(fk::TickCount timeout_ticks) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();

  if (current && current->has_pending_signals())
    return fk::core::Error::Interrupted;

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (current && current->has_pending_signals())
      return fk::core::Error::Interrupted;

    if (!m_pending_bits.is_empty()) {
      fk::NotificationBits bits = m_pending_bits;
      m_pending_bits.clear_all();
      return bits;
    }

    if (current && current->has_pending_signals())
      return fk::core::Error::Interrupted;

    m_waiting_tasks.append(*current);
  }

  scheduler.sleep_current(timeout_ticks);

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    bool still_waiting = current->wait_node.prev != nullptr
                      || current->wait_node.next != nullptr
                      || m_waiting_tasks.first() == current;
    if (still_waiting) {
      m_waiting_tasks.remove(*current);
      return fk::NotificationBits(0);
    }
  }

  if (current && current->has_pending_signals())
    return fk::core::Error::Interrupted;

  uint64_t result = current->registers().rax;
  return fk::NotificationBits(result);
}

fk::NotificationBits Notification::poll() {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  fk::NotificationBits bits = m_pending_bits;
  m_pending_bits.clear_all();

  IpcLogNode::the()->log_notification_operation("poll", 0, bits.value());
  return bits;
}

}
}
