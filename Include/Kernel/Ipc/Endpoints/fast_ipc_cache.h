#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

class Endpoint;

// Per-task cache of the last endpoint capability used for an IPC fastpath
// attempt. Allows the fastpath to skip the CSpace lookup for the common
// case. Validity is tied to the endpoint generation: any revocation bumps
// m_generation on the Endpoint, so a stale cache entry fails the check and
// falls back to the slow path.
struct FastIpcCache {
  Endpoint *endpoint{nullptr};
  uint64_t generation{0};
  uint32_t handle{0};
  uint32_t rights{0};

  bool is_valid() const { return endpoint != nullptr; }

  void reset() {
    endpoint = nullptr;
    generation = 0;
    handle = 0;
    rights = 0;
  }
};

} // namespace ipc
} // namespace fkernel
