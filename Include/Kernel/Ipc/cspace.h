#pragma once

#include <Kernel/Ipc/capability.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

static constexpr uint32_t INVALID_HANDLE = ~uint32_t(0);

class CSpace {
  fk::containers::Vector<Capability> m_capabilities;
  fk::containers::Vector<uint32_t>   m_free_list;

public:
  CSpace() = default;

  uint32_t install(Capability cap) {
    if (!m_free_list.is_empty()) {
      uint32_t idx = m_free_list[m_free_list.size() - 1];
      m_free_list.pop_back();
      m_capabilities[idx] = cap;
      return idx;
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
    if (handle >= m_capabilities.size())
      return;
    m_capabilities[handle] = {};
    m_free_list.push_back(handle);
  }

  bool contains(uint32_t handle) const {
    return handle < m_capabilities.size() && m_capabilities[handle].is_valid();
  }

  uint32_t transfer(CSpace& dest, uint32_t handle, CapabilityRights mask = CapabilityRights::All) {
    auto cap = get(handle);
    if (!cap.is_valid())
      return INVALID_HANDLE;
    remove(handle);
    return dest.install(cap.with_rights(cap.rights() & mask));
  }

  uint32_t grant(CSpace& dest, uint32_t handle, CapabilityRights mask = CapabilityRights::All) {
    auto cap = get(handle);
    if (!cap.is_valid())
      return INVALID_HANDLE;
    return dest.install(cap.with_rights(cap.rights() & mask));
  }

  size_t size() const { return m_capabilities.size() - m_free_list.size(); }

  Capability find_by_object(void* object) const {
    for (size_t i = 0; i < m_capabilities.size(); ++i) {
      if (m_capabilities[i].object() == object && m_capabilities[i].is_valid())
        return m_capabilities[i];
    }
    return {};
  }

  void remove_by_object(void* object) {
    for (size_t i = 0; i < m_capabilities.size(); ++i) {
      if (m_capabilities[i].object() == object) {
        remove(static_cast<uint32_t>(i));
        return;
      }
    }
  }

  void grant_all_to(CSpace& dest, CapabilityType type = CapabilityType::None) {
    for (size_t i = 0; i < m_capabilities.size(); ++i) {
      if (!m_capabilities[i].is_valid()) continue;
      if (type != CapabilityType::None && m_capabilities[i].type() != type) continue;
      dest.install(m_capabilities[i]);
    }
  }
};

}
}
