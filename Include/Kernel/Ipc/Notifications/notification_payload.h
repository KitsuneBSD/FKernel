#pragma once

#include <LibFK/Types/Ipc/notification_bits.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

static constexpr size_t NOTIFICATION_PAYLOAD_SIZE = 128;

struct NotificationPayload {
  fk::NotificationBits bits;
  uint8_t data[NOTIFICATION_PAYLOAD_SIZE];
};

} // namespace ipc
} // namespace fkernel
