#include <Kernel/Ipc/endpoint.h>
#include <Kernel/Ipc/ipc_log_node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/turnstile.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace ipc {

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
  auto* ts = fkernel::scheduler::create_turnstile(SchedulerManager::the().current(), &task);
  fkernel::scheduler::boost_qos_if_needed(&task, SchedulerManager::the().current());
  SchedulerManager::the().wake_task(&task);
  fkernel::scheduler::unboost_task(SchedulerManager::the().current());
  fkernel::scheduler::destroy_turnstile(ts);
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
    m_senders.push_back(current);
  }

  scheduler.sleep_current(timeout_ticks);

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
    m_receivers.push_back(current);
  }

  scheduler.sleep_current(timeout_ticks);

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
      m_senders.push_back(current);
      scheduler.block_current_noqueue();

      m_call_sender = nullptr;
      return MessageInfo(current->registers().rax);
    }

    IpcLogNode::the()->log_endpoint_operation("call_blocked_no_receiver", caller_id, 0, info.raw());
    m_senders.push_back(current);
    scheduler.block_current_noqueue();

    m_call_sender = nullptr;
    return MessageInfo(current->registers().rax);
  }
}

}
}
