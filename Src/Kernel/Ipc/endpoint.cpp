#include <Kernel/Ipc/endpoint.h>
#include <Kernel/Scheduler/scheduler.h>

namespace fkernel {
namespace ipc {

void Endpoint::deliver_message(Task &sender, Task &receiver, MessageInfo info) {
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

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (m_receivers.empty()) {
      m_senders.push_back(current);
      scheduler.block_current();
      return MessageInfo(current->registers().rax); // Result set by deliver_message
    }

    Task *receiver = m_receivers.front();
    m_receivers.remove(receiver);

    deliver_message(*current, *receiver, info);
    scheduler.wake_task(receiver);
  }

  return info;
}

fk::core::Result<MessageInfo> Endpoint::receive() {
  auto &scheduler = SchedulerManager::the();
  Task *current = scheduler.current();

  {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    if (m_senders.empty()) {
      m_receivers.push_back(current);
      scheduler.block_current();
      return MessageInfo(current->registers().rax);
    }

    Task *sender = m_senders.front();
    m_senders.remove(sender);

    MessageInfo info(sender->registers().rax); // Assuming info was in rax
    deliver_message(*sender, *current, info);

    scheduler.wake_task(sender);
    return info;
  }
}

} // namespace ipc
} // namespace fkernel
