#pragma once

#include <Kernel/Ipc/message_info.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Core/result.h>

#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace ipc {

static constexpr uint64_t ENDPOINT_TIMEOUT_FOREVER = ~0ULL;

class Endpoint {
  fk::synchronization::Spinlock m_lock;
  fk::containers::IntrusiveList<Task, &Task::wait_node> m_senders;
  fk::containers::IntrusiveList<Task, &Task::recv_wait_node> m_receivers;
  uint64_t m_generation{0};

  Task* m_call_sender{nullptr};

public:
  Endpoint() = default;

  /// @brief Sends a message through the endpoint. Blocks indefinitely if no receiver is waiting.
  fk::core::Result<MessageInfo> send(MessageInfo info);

  /// @brief Sends with timeout. Returns Error::Timeout on expiry.
  fk::core::Result<MessageInfo> send_timeout(MessageInfo info, uint64_t timeout_ticks);

  /// @brief Receives a message from the endpoint. Blocks indefinitely if no sender is waiting.
  fk::core::Result<MessageInfo> receive();

  /// @brief Receives with timeout. Returns Error::Timeout on expiry.
  fk::core::Result<MessageInfo> receive_timeout(uint64_t timeout_ticks);

  /// @brief Atomic send+receive: sends and immediately waits for reply without
  /// allowing another task to steal the reply. The reply is routed back to the
  /// calling task even if another receiver is waiting.
  fk::core::Result<MessageInfo> call(MessageInfo info);

  /// @brief Revoke all capabilities pointing to this endpoint.
  void revoke() { ++m_generation; }
  const uint64_t* generation_ptr() const { return &m_generation; }
  uint64_t generation() const { return m_generation; }

private:
  void deliver_message(Task &sender, Task &receiver, MessageInfo info);
  void wake_and_unblock(Task &task);
  bool is_on_list(Task& task, fk::containers::IntrusiveListNode<Task> Task::*node_member);
};

}
}
