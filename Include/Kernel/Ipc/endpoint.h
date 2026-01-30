#pragma once

#include <Kernel/Ipc/message_info.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Core/Result.h>

#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace ipc {

class Endpoint {
  fk::synchronization::Spinlock m_lock;
  fk::containers::IntrusiveList<Task, &Task::wait_node> m_senders;
  fk::containers::IntrusiveList<Task, &Task::wait_node> m_receivers;

public:
  Endpoint() = default;

  /// @brief Sends a message through the endpoint. Blocks if no receiver is
  /// waiting.
  fk::core::Result<MessageInfo> send(MessageInfo info);

  /// @brief Receives a message from the endpoint. Blocks if no sender is
  /// waiting.
  fk::core::Result<MessageInfo> receive();

private:
  void deliver_message(Task &sender, Task &receiver, MessageInfo info);
};

} // namespace ipc
} // namespace fkernel
