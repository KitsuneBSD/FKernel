#pragma once

#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>
#include <LibFK/Types/notification_bits.h>
#include <LibFK/Types/tick_count.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/intrusive_list.h>

#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace ipc {

static constexpr size_t NOTIFICATION_PAYLOAD_SIZE = 128;
static constexpr size_t NOTIFICATION_MAX_PAYLOADS = 16;

struct NotificationPayload {
  fk::NotificationBits bits;
  uint8_t data[NOTIFICATION_PAYLOAD_SIZE];
};

class Notification {
  fk::synchronization::Spinlock m_lock;
  fk::NotificationBits m_pending_bits{0};
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

  void signal(fk::NotificationBits bits);

  void signal_with_payload(fk::NotificationBits bits, const void* data, size_t len);

  fk::NotificationBits wait();

  fk::core::Result<fk::NotificationBits, fk::core::Error> wait_interruptible();

  fk::NotificationBits wait_timeout(fk::TickCount timeout_ticks);

  fk::core::Result<fk::NotificationBits, fk::core::Error> wait_interruptible_timeout(fk::TickCount timeout_ticks);

  fk::NotificationBits poll();

  void revoke() { ++m_generation; }
  const uint64_t* generation_ptr() const { return &m_generation; }
  uint64_t generation() const { return m_generation; }
};

}
}
