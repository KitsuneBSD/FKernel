#pragma once

#include <Kernel/Ipc/endpoint.h>
#include <Kernel/Ipc/notification.h>
#include <LibFK/Container/hash_map.h>

namespace fkernel {
namespace ipc {

class GlobalEndpointManager {
  fk::containers::HashMap<uint64_t, Endpoint *> m_endpoints;
  fk::containers::HashMap<uint64_t, Notification *> m_notifications;

  GlobalEndpointManager() = default;

public:
  static GlobalEndpointManager &the() {
    static GlobalEndpointManager instance;
    return instance;
  }

  Endpoint *get_endpoint(uint64_t id) {
    auto opt = m_endpoints.get(id);
    return opt.has_value() ? opt.value() : nullptr;
  }

  Notification *get_notification(uint64_t id) {
    auto opt = m_notifications.get(id);
    return opt.has_value() ? opt.value() : nullptr;
  }

  void register_endpoint(uint64_t id, Endpoint *endpoint) {
    m_endpoints.insert(id, endpoint);
  }

  void register_notification(uint64_t id, Notification *notification) {
    m_notifications.insert(id, notification);
  }

  void remove_notification(uint64_t id) {
    (void)m_notifications.remove(id);
  }

  void remove_endpoint(uint64_t id) {
    (void)m_endpoints.remove(id);
  }
};

} // namespace ipc
} // namespace fkernel
