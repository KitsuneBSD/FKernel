#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Ipc/Endpoints/endpoint.h>
#include <Kernel/Ipc/Endpoints/ipc_log_node.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Sync/turnstile.h>

namespace fkernel {
namespace ipc {

static bool task_cpu_compatible(Task* task) {
  uint64_t cpu_id = get_current_cpu_id();
  uint64_t affinity = task->control.lifecycle.cpu_affinity;
  return affinity == 0 || (affinity & (1ULL << cpu_id)) != 0;
}

static bool fastpath_qos_ok(Task* caller, Task* receiver) {
  // Fastpath only when receiver is at least as urgent as caller (lower value = higher priority).
  return (uint8_t)receiver->control.lifecycle.qos <= (uint8_t)caller->control.lifecycle.qos;
}

static void unboost_current_if_boosted() {
    Task* current = SchedulerManager::the().current();
    if (current && current->control.lifecycle.boosted) {
        fkernel::scheduler::unboost_task(current);
    }
}

// Copies the message into the receiver's rendezvous slot. The receiver reads
// it back after schedule() resumes it — the PtRegs frame on the kernel stack
// is the actual restore source on sysret, so the syscall handler copies the
// slot into regs before returning.
void Endpoint::deliver_message(Task& sender, Task& receiver, MessageInfo info,
                               const IpcMessage& message) {
  uint32_t sender_id = sender.control.identity.id.value();
  uint32_t receiver_id = receiver.control.identity.id.value();

  IpcLogNode::the()->log_endpoint_operation("deliver_message", sender_id, receiver_id, info.raw());

  receiver.ipc().pending_info = info;
  receiver.ipc().pending_message = message;
}

void Endpoint::wake_and_unblock(Task& task) {
  fkernel::scheduler::boost_qos_if_needed(&task, SchedulerManager::the().current());
  SchedulerManager::the().wake_task(&task);
}

bool Endpoint::is_on_list(Task& task, fk::containers::IntrusiveListNode<Task> Task::*node_member) {
  auto& node = task.*node_member;
  return node.prev != nullptr || node.next != nullptr;
}

fk::core::Result<MessageInfo> Endpoint::send(MessageInfo info, const IpcMessage& message) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t sender_id = current->control.identity.id.value();
  bool should_block = false;
  Task* fastpath_caller = nullptr;

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);

    if (m_call_sender != nullptr) {
      Task* caller = m_call_sender;
      m_call_sender = nullptr;
      uint32_t caller_id = caller->control.identity.id.value();
      deliver_message(*current, *caller, info, message);

      bool can_fastpath = !caller->has_pending_signals() && task_cpu_compatible(caller);
      if (can_fastpath) {
        IpcLogNode::the()->log_endpoint_operation("send_reply_fastpath", sender_id, caller_id, info.raw());
        fastpath_caller = caller;
      } else {
        IpcLogNode::the()->log_endpoint_operation("send_reply", sender_id, caller_id, info.raw());
        wake_and_unblock(*caller);
      }

    } else if (!m_receivers.empty()) {
      Task* receiver = m_receivers.front();
      m_receivers.remove(receiver);
      uint32_t receiver_id = receiver->control.identity.id.value();
      IpcLogNode::the()->log_endpoint_operation("send_immediate", sender_id, receiver_id, info.raw());
      deliver_message(*current, *receiver, info, message);
      wake_and_unblock(*receiver);

    } else {
      IpcLogNode::the()->log_endpoint_operation("send_blocked", sender_id, 0, info.raw());
      unboost_current_if_boosted();
      current->ipc().pending_info = info;
      current->ipc().pending_message = message;
      m_senders.push_back(current);
      scheduler.block_current_noqueue();
      should_block = true;
    }
  }

  if (fastpath_caller) {
    // Reply fastpath: put server back on run-queue then switch directly to client.
    scheduler.requeue_running_task();
    scheduler.switch_to_task(fastpath_caller);
  } else if (should_block) {
    scheduler.schedule();
  }

  return info;
}

fk::core::Result<MessageInfo> Endpoint::send_timeout(MessageInfo info, const IpcMessage& message,
                                                     uint64_t timeout_ticks) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t sender_id = current->control.identity.id.value();
  bool should_sleep = false;

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (m_call_sender != nullptr) {
      Task* caller = m_call_sender;
      m_call_sender = nullptr;
      uint32_t caller_id = caller->control.identity.id.value();

      IpcLogNode::the()->log_endpoint_operation("send_timeout_reply", sender_id, caller_id, info.raw());
      deliver_message(*current, *caller, info, message);
      wake_and_unblock(*caller);
      return info;
    }

    if (!m_receivers.empty()) {
      Task* receiver = m_receivers.front();
      m_receivers.remove(receiver);
      uint32_t receiver_id = receiver->control.identity.id.value();

      IpcLogNode::the()->log_endpoint_operation("send_timeout_immediate", sender_id, receiver_id, info.raw());
      deliver_message(*current, *receiver, info, message);
      wake_and_unblock(*receiver);
      return info;
    }

    if (timeout_ticks == 0) {
      IpcLogNode::the()->log_endpoint_operation("send_timeout_nonblock", sender_id, 0, info.raw());
      return fk::core::Error::Timeout;
    }

    IpcLogNode::the()->log_endpoint_operation("send_timeout_blocked", sender_id, 0, info.raw());
    unboost_current_if_boosted();
    current->ipc().pending_info = info;
    current->ipc().pending_message = message;
    m_senders.push_back(current);
    should_sleep = true;
  }

  if (should_sleep) {
    scheduler.sleep_current(fk::TickCount(timeout_ticks));
    scheduler.schedule();
  }

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (is_on_list(*current, &Task::wait_node)) {
      m_senders.remove(*current);
      IpcLogNode::the()->log_endpoint_operation("send_timeout_expired", sender_id, 0, 0);
      return fk::core::Error::Timeout;
    }
  }

  return info;
}

fk::core::Result<MessageInfo> Endpoint::receive() {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t receiver_id = current->control.identity.id.value();
  bool should_block = false;

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);

    if (!m_senders.empty()) {
      Task* sender = m_senders.front();
      m_senders.remove(sender);
      uint32_t sender_id = sender->control.identity.id.value();

      bool is_call_sender = (sender == m_call_sender);
      MessageInfo info = sender->ipc().pending_info;
      IpcMessage message = sender->ipc().pending_message;

      IpcLogNode::the()->log_endpoint_operation("receive_immediate", receiver_id, sender_id, info.raw());
      deliver_message(*sender, *current, info, message);

      // A call sender must stay blocked until it receives a reply; only a
      // plain message sender is woken by having its message accepted.
      if (!is_call_sender)
        wake_and_unblock(*sender);
      return info;
    }

    IpcLogNode::the()->log_endpoint_operation("receive_blocked", receiver_id, 0, 0);
    unboost_current_if_boosted();
    m_receivers.push_back(current);
    scheduler.block_current_noqueue();
    should_block = true;
  }

  if (should_block)
    scheduler.schedule();

  return current->ipc().pending_info;
}

fk::core::Result<MessageInfo> Endpoint::receive_timeout(uint64_t timeout_ticks) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t receiver_id = current->control.identity.id.value();
  bool should_sleep = false;

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (!m_senders.empty()) {
      Task* sender = m_senders.front();
      m_senders.remove(sender);
      uint32_t sender_id = sender->control.identity.id.value();

      bool is_call_sender = (sender == m_call_sender);
      MessageInfo info = sender->ipc().pending_info;
      IpcMessage message = sender->ipc().pending_message;

      IpcLogNode::the()->log_endpoint_operation("receive_timeout_immediate", receiver_id, sender_id, info.raw());
      deliver_message(*sender, *current, info, message);

      if (!is_call_sender)
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
    should_sleep = true;
  }

  if (should_sleep) {
    scheduler.sleep_current(fk::TickCount(timeout_ticks));
    scheduler.schedule();
  }

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (is_on_list(*current, &Task::recv_wait_node)) {
      m_receivers.remove(*current);
      IpcLogNode::the()->log_endpoint_operation("receive_timeout_expired", receiver_id, 0, 0);
      return fk::core::Error::Timeout;
    }
  }

  return current->ipc().pending_info;
}

fk::core::Result<MessageInfo> Endpoint::call(MessageInfo info, const IpcMessage& message) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  uint32_t caller_id = current->control.identity.id.value();
  bool should_block = false;
  Task* fastpath_receiver = nullptr;

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);

    m_call_sender = current;

    if (!m_receivers.empty()) {
      Task* receiver = m_receivers.front();
      m_receivers.remove(receiver);
      uint32_t receiver_id = receiver->control.identity.id.value();

      bool can_fastpath = !current->has_pending_signals() &&
                          !receiver->has_pending_signals() &&
                          task_cpu_compatible(receiver) &&
                          fastpath_qos_ok(current, receiver);

      deliver_message(*current, *receiver, info, message);

      if (can_fastpath) {
        IpcLogNode::the()->log_endpoint_operation("call_fastpath", caller_id, receiver_id, info.raw());
        unboost_current_if_boosted();
        scheduler.block_current_noqueue();
        fastpath_receiver = receiver;
      } else {
        IpcLogNode::the()->log_endpoint_operation("call_deliver", caller_id, receiver_id, info.raw());
        wake_and_unblock(*receiver);
        unboost_current_if_boosted();
        scheduler.block_current_noqueue();
        should_block = true;
      }

    } else {
      // No server waiting yet: queue in senders. receive() accepts the message
      // and keeps us blocked waiting for the reply (via m_call_sender).
      IpcLogNode::the()->log_endpoint_operation("call_blocked_no_receiver", caller_id, 0, info.raw());
      current->ipc().pending_info = info;
      current->ipc().pending_message = message;
      m_senders.push_back(current);
      unboost_current_if_boosted();
      scheduler.block_current_noqueue();
      should_block = true;
    }
  }

  if (fastpath_receiver)
    scheduler.switch_to_task(fastpath_receiver);
  else if (should_block)
    scheduler.schedule();

  if (m_call_sender == current)
    m_call_sender = nullptr;

  return current->ipc().pending_info;
}

// --- Async API ---

void Endpoint::signal(fk::NotificationBits bits) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_pending_bits |= bits;

  if (!m_async_waiters.is_empty()) {
    Task& task = *m_async_waiters.first();
    m_async_waiters.remove(task);

    task.ipc().pending_notification = m_pending_bits;
    uint64_t delivered_bits = m_pending_bits.value();
    m_pending_bits.clear_all();

    IpcLogNode::the()->log_notification_operation("signal_wake", task.control.identity.id.value(), delivered_bits);
    wake_and_unblock(task);
  }
}

fk::NotificationBits Endpoint::wait() {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  bool should_block = false;

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
    should_block = true;
  }

  if (should_block)
    scheduler.schedule();

  return current->ipc().pending_notification;
}

fk::core::Result<fk::NotificationBits, fk::core::Error> Endpoint::wait_interruptible() {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  bool should_block = false;

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
    should_block = true;
  }

  if (should_block)
    scheduler.schedule();

  if (current && current->has_pending_signals())
    return fk::core::Error::Interrupted;
  return current->ipc().pending_notification;
}

fk::NotificationBits Endpoint::wait_timeout(fk::TickCount timeout_ticks) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  bool should_sleep = false;

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (!m_pending_bits.is_empty()) {
      fk::NotificationBits bits = m_pending_bits;
      m_pending_bits.clear_all();
      return bits;
    }

    m_async_waiters.append(*current);
    should_sleep = true;
  }

  if (should_sleep) {
    scheduler.sleep_current(timeout_ticks);
    scheduler.schedule();
  }

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    bool still_waiting = is_on_list(*current, &Task::wait_node) || m_async_waiters.first() == current;
    if (still_waiting) {
      m_async_waiters.remove(*current);
      return fk::NotificationBits(0);
    }
    // Signal arrived — read the delivered bits from the slot.
    fk::NotificationBits bits = current->ipc().pending_notification;
    return bits;
  }
}

fk::core::Result<fk::NotificationBits, fk::core::Error> Endpoint::wait_interruptible_timeout(fk::TickCount timeout_ticks) {
  auto& scheduler = SchedulerManager::the();
  Task* current = scheduler.current();
  bool should_sleep = false;

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
    should_sleep = true;
  }

  if (should_sleep) {
    scheduler.sleep_current(timeout_ticks);
    scheduler.schedule();
  }

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
    fk::NotificationBits bits = current->ipc().pending_notification;
    return bits;
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

    task.ipc().pending_notification = m_pending_bits;
    uint64_t delivered_bits = m_pending_bits.value();
    m_pending_bits.clear_all();

    IpcLogNode::the()->log_notification_operation("signal_wake_payload", task.control.identity.id.value(), delivered_bits);
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
