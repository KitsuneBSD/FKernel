#pragma once

#include <Kernel/Ipc/message_info.h>
#include <Kernel/Ipc/notification.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/notification_bits.h>
#include <LibFK/Types/tick_count.h>

#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace ipc {

static constexpr uint64_t ENDPOINT_TIMEOUT_FOREVER = ~0ULL;

class Endpoint {
  fk::synchronization::Spinlock m_lock;
  fk::containers::IntrusiveList<Task, &Task::wait_node> m_senders;
  fk::containers::IntrusiveList<Task, &Task::recv_wait_node> m_receivers;
  fk::containers::IntrusiveList<Task, &Task::wait_node> m_async_waiters;
  uint64_t m_generation{0};

  fk::NotificationBits m_pending_bits{0};

  Task* m_call_sender{nullptr};

  NotificationPayload m_payloads[NOTIFICATION_MAX_PAYLOADS];
  uint8_t m_payload_head{0};
  uint8_t m_payload_tail{0};
  uint8_t m_payload_count{0};

  bool payload_pop(NotificationPayload& out);
  void payload_push(const NotificationPayload& p);

public:
  Endpoint() = default;

  // --- Synchronous rendezvous API (seL4-style) ---

  fk::core::Result<MessageInfo> send(MessageInfo info);

  fk::core::Result<MessageInfo> send_timeout(MessageInfo info, uint64_t timeout_ticks);

  fk::core::Result<MessageInfo> receive();

  fk::core::Result<MessageInfo> receive_timeout(uint64_t timeout_ticks);

  fk::core::Result<MessageInfo> call(MessageInfo info);

  // --- Asynchronous signal/wait API (Notification-compatible) ---

  void signal(fk::NotificationBits bits);

  void signal_with_payload(fk::NotificationBits bits, const void* data, size_t len);

  fk::NotificationBits wait();

  fk::NotificationBits wait_timeout(fk::TickCount timeout_ticks);

  fk::NotificationBits poll();

  // --- Revocation ---

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
