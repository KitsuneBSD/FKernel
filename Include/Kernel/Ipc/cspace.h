#pragma once

#include <Kernel/Ipc/capability.h>
#include <LibFK/Container/hash_map.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

static constexpr uint32_t INVALID_HANDLE = ~uint32_t(0);

class CSpace {
  fk::containers::Vector<Capability>    m_capabilities;
  fk::containers::Vector<uint32_t>      m_free_list;
  // Reverse index: object pointer → slot index (O(1) find/remove by object)
  fk::containers::HashMap<void*, uint32_t> m_obj_index;

public:
  CSpace() = default;

  uint32_t install(Capability cap) {
    uint32_t idx;
    if (!m_free_list.is_empty()) {
      idx = m_free_list[m_free_list.size() - 1];
      m_free_list.pop_back();
      m_capabilities[idx] = cap;
    } else {
      m_capabilities.push_back(cap);
      idx = static_cast<uint32_t>(m_capabilities.size() - 1);
    }
    if (cap.object()) m_obj_index.insert(cap.object(), idx);
    return idx;
  }

  Capability get(uint32_t handle) const {
    if (handle >= m_capabilities.size())
      return {};
    return m_capabilities[handle];
  }

  void remove(uint32_t handle) {
    if (handle >= m_capabilities.size()) return;
    void* obj = m_capabilities[handle].object();
    if (obj) m_obj_index.remove(obj);
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
    auto idx = m_obj_index.get(object);
    if (!idx.has_value()) return {};
    return m_capabilities[idx.value()];
  }

  void remove_by_object(void* object) {
    auto idx = m_obj_index.get(object);
    if (idx.has_value()) remove(idx.value());
  }

  uint32_t install_fd(void* desc, CapabilityRights rights) {
    return install(Capability(desc, CapabilityType::FileDescription, rights));
  }

  void* lookup_fd(uint32_t handle) const {
    auto cap = get(handle);
    if (!cap.is_valid()) return nullptr;
    if (cap.type() != CapabilityType::FileDescription) return nullptr;
    return cap.object();
  }

  void revoke_fd(uint32_t handle) {
    auto cap = get(handle);
    if (cap.type() == CapabilityType::FileDescription)
      remove(handle);
  }

  void grant_all_to(CSpace& dest, CapabilityType type = CapabilityType::None) {
    for (size_t i = 0; i < m_capabilities.size(); ++i) {
      if (!m_capabilities[i].is_valid()) continue;
      if (type != CapabilityType::None && m_capabilities[i].type() != type) continue;
      dest.install(m_capabilities[i]);
    }
  }

  // Copies one FileDescriptor capability from this CSpace into dest, preserving rights.
  // Returns the new handle in dest, or INVALID_HANDLE if src_handle is invalid.
  uint32_t clone_fd(CSpace& dest, uint32_t src_handle) const {
    if (src_handle == INVALID_HANDLE) return INVALID_HANDLE;
    auto cap = get(src_handle);
    if (!cap.is_valid() || cap.type() != CapabilityType::FileDescription)
      return INVALID_HANDLE;
    return dest.install(cap);
  }
};

}
}
