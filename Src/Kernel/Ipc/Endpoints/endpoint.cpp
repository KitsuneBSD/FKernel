#include <Kernel/Ipc/Endpoints/endpoint.h>
#include <Kernel/Ipc/Endpoints/ipc_log_node.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Sync/turnstile.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {
namespace ipc {

static void unboost_current_if_boosted() {
    Task* current = SchedulerManager::the().current();
    if (current && current->control.lifecycle.boosted) {
        fkernel::scheduler::unboost_task(current);
    }
}

void Endpoint::deliver_message(Task& sender, Task& receiver, MessageInfo info) {
  uint32_t sender_id = sender.control.identity.id.value();
  uint32_t receiver_id = receiver.control.identity.id.value();

  IpcLogNode::the()->log_endpoint_operation("deliver_message", sender_id, receiver_id, info.raw());

  receiver.registers().rdi = sender.registers().rdi;
  receiver.registers().rsi = sender.registers().rsi;
  receiver.registers().rdx = sender.registers().rdx;
  receiver.registers().r10 = sender.registers().r10;
  receiver.registers().r8 = sender.registers().r8;
  receiver.registers().r9 = sender.registers().r9;

  receiver.registers().rax = info.raw();
}

void Endpoint::wake_and_unblock(Task& task) {
  fkernel::scheduler::boost_qos_if_needed(&task, SchedulerManager::the().current());
  SchedulerManager::the().wake_task(&task);
}

bool Endpoint::is_on_list(Task& task, fk::containers::IntrusiveListNode<Task> Task::*node_member) {
  auto& node = task.*node_member;
  return node.prev != nullptr || node.next != nullptr;
}

fk::core::Result<MessageInfo> Endpoint::send(MessageInfo info) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t sender_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (m_receivers.empty() && m_call_sender == nullptr) {
      IpcLogNode::the()->log_endpoint_operation("send_blocked", sender_id, 0, info.raw());
      unboost_current_if_boosted();
      m_senders.push_back(current);
      scheduler.block_current_noqueue();
      return MessageInfo(current->registers().rax);
    }

    Task* receiver = nullptr;
    if (m_call_sender == current) {
      receiver = m_receivers.empty() ? nullptr : m_receivers.front();
      m_call_sender = nullptr;
      if (receiver)
        m_receivers.remove(receiver);
    } else {
      receiver = m_receivers.front();
      m_receivers.remove(receiver);
    }

    if (receiver) {
      uint32_t receiver_id = receiver->control.identity.id.value();
      IpcLogNode::the()->log_endpoint_operation("send_immediate", sender_id, receiver_id, info.raw());

      deliver_message(*current, *receiver, info);
      wake_and_unblock(*receiver);
    }
  }

  return info;
}

fk::core::Result<MessageInfo> Endpoint::send_timeout(MessageInfo info, uint64_t timeout_ticks) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t sender_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (!m_receivers.empty()) {
      Task* receiver = m_receivers.front();
      m_receivers.remove(receiver);
      uint32_t receiver_id = receiver->control.identity.id.value();
      IpcLogNode::the()->log_endpoint_operation("send_timeout_immediate", sender_id, receiver_id, info.raw());

      deliver_message(*current, *receiver, info);
      wake_and_unblock(*receiver);
      return info;
    }

    if (timeout_ticks == 0) {
      IpcLogNode::the()->log_endpoint_operation("send_timeout_nonblock", sender_id, 0, info.raw());
      return fk::core::Error::Timeout;
    }

    IpcLogNode::the()->log_endpoint_operation("send_timeout_blocked", sender_id, 0, info.raw());
    unboost_current_if_boosted();
    m_senders.push_back(current);
  }

  scheduler.sleep_current(fk::TickCount(timeout_ticks));

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (is_on_list(*current, &Task::wait_node)) {
      m_senders.remove(*current);
      IpcLogNode::the()->log_endpoint_operation("send_timeout_expired", sender_id, 0, 0);
      return fk::core::Error::Timeout;
    }
  }

  return MessageInfo(current->registers().rax);
}

fk::core::Result<MessageInfo> Endpoint::receive() {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t receiver_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (m_senders.empty()) {
      IpcLogNode::the()->log_endpoint_operation("receive_blocked", receiver_id, 0, 0);
      unboost_current_if_boosted();
      m_receivers.push_back(current);
      scheduler.block_current();
      return MessageInfo(current->registers().rax);
    }

    Task* sender = m_senders.front();
    m_senders.remove(sender);
    uint32_t sender_id = sender->control.identity.id.value();

    MessageInfo info(sender->registers().rax);
    IpcLogNode::the()->log_endpoint_operation("receive_immediate", receiver_id, sender_id, info.raw());

    deliver_message(*sender, *current, info);
    wake_and_unblock(*sender);
    return info;
  }
}

fk::core::Result<MessageInfo> Endpoint::receive_timeout(uint64_t timeout_ticks) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t receiver_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (!m_senders.empty()) {
      Task* sender = m_senders.front();
      m_senders.remove(sender);
      uint32_t sender_id = sender->control.identity.id.value();

      MessageInfo info(sender->registers().rax);
      IpcLogNode::the()->log_endpoint_operation("receive_timeout_immediate", receiver_id, sender_id, info.raw());

      deliver_message(*sender, *current, info);
      wake_and_unblock(*sender);
      return info;
    }

    if (timeout_ticks == 0) {
      IpcLogNode::the()->log_endpoint_operation("receive_timeout_nonblock", receiver_id, 0, 0);
      return fk::core::Error::Timeout;
    }

    IpcLogNode::the()->log_endpoint_operation("receive_timeout_blocked", receiver_id, 0, 0);
    unboost_current_if_boosted();
    m_receivers.push_back(current);
  }

  scheduler.sleep_current(fk::TickCount(timeout_ticks));

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (is_on_list(*current, &Task::recv_wait_node)) {
      m_receivers.remove(*current);
      IpcLogNode::the()->log_endpoint_operation("receive_timeout_expired", receiver_id, 0, 0);
      return fk::core::Error::Timeout;
    }
  }

  return MessageInfo(current->registers().rax);
}

fk::core::Result<MessageInfo> Endpoint::call(MessageInfo info) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t caller_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);

    m_call_sender = current;

    if (!m_receivers.empty()) {
      Task* receiver = m_receivers.front();
      m_receivers.remove(receiver);
      uint32_t receiver_id = receiver->control.identity.id.value();

      IpcLogNode::the()->log_endpoint_operation("call_deliver", caller_id, receiver_id, info.raw());
      deliver_message(*current, *receiver, info);
      wake_and_unblock(*receiver);

      IpcLogNode::the()->log_endpoint_operation("call_waiting_reply", caller_id, 0, 0);
      unboost_current_if_boosted();
      m_senders.push_back(current);
      scheduler.block_current_noqueue();

      m_call_sender = nullptr;
      return MessageInfo(current->registers().rax);
    }

    IpcLogNode::the()->log_endpoint_operation("call_blocked_no_receiver", caller_id, 0, info.raw());
    unboost_current_if_boosted();
    m_senders.push_back(current);
    scheduler.block_current_noqueue();

    m_call_sender = nullptr;
    return MessageInfo(current->registers().rax);
  }
}

// --- Async API ---

void Endpoint::signal(fk::NotificationBits bits) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_pending_bits |= bits;

  if (!m_async_waiters.is_empty()) {
    Task& task = *m_async_waiters.first();
    m_async_waiters.remove(task);
    wake_and_unblock(task);
  }
}

fk::NotificationBits Endpoint::wait() {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (!m_pending_bits.is_empty()) {
      fk::NotificationBits bits = m_pending_bits;
      m_pending_bits.clear_all();
      return bits;
    }

    m_async_waiters.append(*current);
    unboost_current_if_boosted();
    scheduler.block_current_noqueue();
  }

  // Re-acquire lock after wake-up to safely read pending bits
  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    fk::NotificationBits bits = m_pending_bits;
    m_pending_bits.clear_all();
    return bits;
  }
}

fk::core::Result<fk::NotificationBits, fk::core::Error> Endpoint::wait_interruptible() {
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

    m_async_waiters.append(*current);
    unboost_current_if_boosted();
    scheduler.block_current_noqueue();
  }

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (!m_pending_bits.is_empty()) {
      fk::NotificationBits bits = m_pending_bits;
      m_pending_bits.clear_all();
      return bits;
    }
    if (current && current->has_pending_signals())
      return fk::core::Error::Interrupted;
    return fk::NotificationBits(0);
  }
}

fk::NotificationBits Endpoint::wait_timeout(fk::TickCount timeout_ticks) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (!m_pending_bits.is_empty()) {
      fk::NotificationBits bits = m_pending_bits;
      m_pending_bits.clear_all();
      return bits;
    }

    m_async_waiters.append(*current);
  }

  scheduler.sleep_current(timeout_ticks);

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    bool still_waiting = is_on_list(*current, &Task::wait_node) || m_async_waiters.first() == current;
    if (still_waiting) {
      m_async_waiters.remove(*current);
      return fk::NotificationBits(0);
    }
    // Signal arrived — read pending bits under lock
    fk::NotificationBits bits = m_pending_bits;
    m_pending_bits.clear_all();
    return bits;
  }
}

fk::core::Result<fk::NotificationBits, fk::core::Error> Endpoint::wait_interruptible_timeout(fk::TickCount timeout_ticks) {
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

    m_async_waiters.append(*current);
  }

  scheduler.sleep_current(timeout_ticks);

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    bool still_waiting = is_on_list(*current, &Task::wait_node) || m_async_waiters.first() == current;
    if (still_waiting) {
      m_async_waiters.remove(*current);
      return fk::NotificationBits(0);
    }
    if (!m_pending_bits.is_empty()) {
      fk::NotificationBits bits = m_pending_bits;
      m_pending_bits.clear_all();
      return bits;
    }
    if (current && current->has_pending_signals())
      return fk::core::Error::Interrupted;
    return fk::NotificationBits(0);
  }
}

fk::NotificationBits Endpoint::poll() {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  fk::NotificationBits bits = m_pending_bits;
  m_pending_bits.clear_all();
  return bits;
}

void Endpoint::signal_with_payload(fk::NotificationBits bits, const void* data, size_t len) {
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

  if (!m_async_waiters.is_empty()) {
    Task& task = *m_async_waiters.first();
    m_async_waiters.remove(task);
    wake_and_unblock(task);
  }
}

bool Endpoint::payload_pop(NotificationPayload& out) {
  if (m_payload_count == 0) return false;
  out = m_payloads[m_payload_head];
  m_payload_head = (m_payload_head + 1) % NOTIFICATION_MAX_PAYLOADS;
  --m_payload_count;
  return true;
}

void Endpoint::payload_push(const NotificationPayload& p) {
  if (m_payload_count >= NOTIFICATION_MAX_PAYLOADS) {
    m_payload_head = (m_payload_head + 1) % NOTIFICATION_MAX_PAYLOADS;
    --m_payload_count;
  }
  m_payloads[m_payload_tail] = p;
  m_payload_tail = (m_payload_tail + 1) % NOTIFICATION_MAX_PAYLOADS;
  ++m_payload_count;
}

}
}
