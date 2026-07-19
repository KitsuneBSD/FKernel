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
  fk::containers::IntrusiveList<Task, &Task::recv_wait_node> m_receivers;
  uint64_t m_generation{0};

public:
  Endpoint() = default;

  /// @brief Sends a message through the endpoint. Blocks if no receiver is
  /// waiting.
  fk::core::Result<MessageInfo> send(MessageInfo info);

  /// @brief Receives a message from the endpoint. Blocks if no sender is
  /// waiting.
  fk::core::Result<MessageInfo> receive();

  /// @brief Revoke all capabilities pointing to this endpoint.
  void revoke() { ++m_generation; }
  const uint64_t* generation_ptr() const { return &m_generation; }
  uint64_t generation() const { return m_generation; }

private:
  void deliver_message(Task &sender, Task &receiver, MessageInfo info);
};

} // namespace ipc
} // namespace fkernel
