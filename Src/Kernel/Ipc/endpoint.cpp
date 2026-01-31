#include <Kernel/Ipc/endpoint.h>
#include <Kernel/Ipc/ipc_log_node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace ipc {

void Endpoint::deliver_message(Task &sender, Task &receiver, MessageInfo info) {
  uint32_t sender_id = sender.control.identity.id.value();
  uint32_t receiver_id = receiver.control.identity.id.value();
  
  fk::algorithms::klog("IPC", "Delivering message from Task %u to %u (Info: 0x%lx)", 
                       sender_id, receiver_id, info.raw());
  IpcLogNode::the()->log_endpoint_operation("deliver_message", sender_id, receiver_id, info.raw());

  // 1. Copy Short Message (Registers)
  // We assume registers rdi-r9 are used for short message data
  receiver.registers().rdi = sender.registers().rdi;
  receiver.registers().rsi = sender.registers().rsi;
  receiver.registers().rdx = sender.registers().rdx;
  receiver.registers().r10 = sender.registers().r10;
  receiver.registers().r8 = sender.registers().r8;
  receiver.registers().r9 = sender.registers().r9;

  // 2. Set the result MessageInfo in rax for the receiver
  receiver.registers().rax = info.raw();

  // 3. Handle Large Message (Shared Memory Copy)
  // TODO: Implement copy between IPC Buffers if info.length() > 0
}

fk::core::Result<MessageInfo> Endpoint::send(MessageInfo info) {
  auto &scheduler = SchedulerManager::the();
  Task *current = scheduler.current();
  uint32_t sender_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (m_receivers.empty()) {
      fk::algorithms::klog("IPC", "Endpoint %p: Task %u blocked on send (Label: 0x%lx)", 
                           this, sender_id, info.label());
      IpcLogNode::the()->log_endpoint_operation("send_blocked", sender_id, 0, info.raw());
      m_senders.push_back(current);
      scheduler.block_current();
      return MessageInfo(current->registers().rax); // Result set by deliver_message
    }

    Task *receiver = m_receivers.front();
    m_receivers.remove(receiver);
    uint32_t receiver_id = receiver->control.identity.id.value();

    fk::algorithms::klog("IPC", "Endpoint %p: Task %u sent immediate message to %u (Label: 0x%lx)", 
                         this, sender_id, receiver_id, info.label());
    IpcLogNode::the()->log_endpoint_operation("send_immediate", sender_id, receiver_id, info.raw());
    deliver_message(*current, *receiver, info);
    scheduler.wake_task(receiver);
  }

  return info;
}

fk::core::Result<MessageInfo> Endpoint::receive() {
  auto &scheduler = SchedulerManager::the();
  Task *current = scheduler.current();
  uint32_t receiver_id = current->control.identity.id.value();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (m_senders.empty()) {
      fk::algorithms::klog("IPC", "Endpoint %p: Task %u blocked on receive", this, receiver_id);
      IpcLogNode::the()->log_endpoint_operation("receive_blocked", receiver_id, 0, 0);
      m_receivers.push_back(current);
      scheduler.block_current();
      return MessageInfo(current->registers().rax);
    }

    Task *sender = m_senders.front();
    m_senders.remove(sender);
    uint32_t sender_id = sender->control.identity.id.value();

    MessageInfo info(sender->registers().rax); // Assuming info was in rax
    fk::algorithms::klog("IPC", "Endpoint %p: Task %u received immediate message from %u (Label: 0x%lx)", 
                         this, receiver_id, sender_id, info.label());
    IpcLogNode::the()->log_endpoint_operation("receive_immediate", receiver_id, sender_id, info.raw());
    deliver_message(*sender, *current, info);

    scheduler.wake_task(sender);
    return info;
  }
}

} // namespace ipc
} // namespace fkernel
