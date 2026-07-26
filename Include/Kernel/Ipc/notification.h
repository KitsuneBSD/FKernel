#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/intrusive_list.h>

#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace ipc {

static constexpr size_t NOTIFICATION_PAYLOAD_SIZE = 64;
static constexpr size_t NOTIFICATION_MAX_PAYLOADS = 16;

struct NotificationPayload {
  uint64_t bits;
  uint8_t data[NOTIFICATION_PAYLOAD_SIZE];
};

class Notification {
  fk::synchronization::Spinlock m_lock;
  uint64_t m_pending_bits{0};
  fk::containers::IntrusiveList<Task, &Task::wait_node> m_waiting_tasks;
  uint64_t m_generation{0};

  NotificationPayload m_payloads[NOTIFICATION_MAX_PAYLOADS];
  uint8_t m_payload_head{0};
  uint8_t m_payload_tail{0};
  uint8_t m_payload_count{0};

  bool payload_pop(NotificationPayload& out);
  void payload_push(const NotificationPayload& p);

public:
  Notification() = default;

  /// @brief Non-blocking signal. Sets the bits and wakes up one waiting task.
  void signal(uint64_t bits);

  /// @brief Signal with attached payload data (up to 64 bytes).
  /// Pushes payload to an internal 16-slot queue for retrieval via wait()/poll().
  void signal_with_payload(uint64_t bits, const void* data, size_t len);

  /// @brief Blocks until at least one bit is set. Returns the pending bits and clears them.
  uint64_t wait();

  /// @brief Blocks with timeout. Returns delivered bits or 0 on timeout.
  /// @param timeout_ticks Maximum ticks to wait; 0 = non-blocking poll.
  uint64_t wait_timeout(uint64_t timeout_ticks);

  /// @brief Non-blocking poll.
  uint64_t poll();

  /// @brief Revoke all capabilities pointing to this notification.
  void revoke() { ++m_generation; }
  const uint64_t* generation_ptr() const { return &m_generation; }
  uint64_t generation() const { return m_generation; }
};

}
}
