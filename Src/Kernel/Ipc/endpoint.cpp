#include <Kernel/Ipc/endpoint.h>
#include <Kernel/Scheduler/scheduler.h>

namespace fkernel {
namespace ipc {

void Endpoint::deliver_message(Task &sender, Task &receiver, MessageInfo info) {
  // 1. Copy Short Message (Registers)
  // We assume registers rdi-r9 are used for short message data
  receiver.context.rdi = sender.context.rdi;
  receiver.context.rsi = sender.context.rsi;
  receiver.context.rdx = sender.context.rdx;
  receiver.context.r10 = sender.context.r10;
  receiver.context.r8 = sender.context.r8;
  receiver.context.r9 = sender.context.r9;

  // 2. Set the result MessageInfo in rax for the receiver
  receiver.context.rax = info.raw();

  // 3. Handle Large Message (Shared Memory Copy)
  // TODO: Implement copy between IPC Buffers if info.length() > 0
}

fk::core::Result<MessageInfo> Endpoint::send(MessageInfo info) {
  auto &scheduler = SchedulerManager::the();
  Task *current = scheduler.current();

  if (m_receivers.empty()) {
    m_senders.push_back(current);
    scheduler.block_current();
    return MessageInfo(current->context.rax); // Result set by deliver_message
  }

  Task *receiver = m_receivers.front();
  m_receivers.remove(receiver);

  deliver_message(*current, *receiver, info);
  scheduler.wake_task(receiver);

  return info;
}

fk::core::Result<MessageInfo> Endpoint::receive() {
  auto &scheduler = SchedulerManager::the();
  Task *current = scheduler.current();

  if (m_senders.empty()) {
    m_receivers.push_back(current);
    scheduler.block_current();
    return MessageInfo(current->context.rax);
  }

  Task *sender = m_senders.front();
  m_senders.remove(sender);

  MessageInfo info(sender->context.rax); // Assuming info was in rax
  deliver_message(*sender, *current, info);

  scheduler.wake_task(sender);
  return info;
}

} // namespace ipc
} // namespace fkernel
