#pragma once

#include <Kernel/Ipc/capability.h>
#include <LibFK/Container/vector.h>

namespace fkernel {
namespace ipc {

class CSpace {
  fk::containers::Vector<Capability> m_capabilities;

public:
  CSpace() = default;

  uint32_t install(Capability cap) {
    for (size_t i = 0; i < m_capabilities.size(); ++i) {
      if (!m_capabilities[i].is_valid()) {
        m_capabilities[i] = cap;
        return static_cast<uint32_t>(i);
      }
    }
    m_capabilities.push_back(cap);
    return static_cast<uint32_t>(m_capabilities.size() - 1);
  }

  Capability get(uint32_t handle) const {
    if (handle >= m_capabilities.size())
      return {};
    return m_capabilities[handle];
  }

  void remove(uint32_t handle) {
    if (handle < m_capabilities.size()) {
      m_capabilities[handle] = {};
    }
  }
};

} // namespace ipc
} // namespace fkernel
