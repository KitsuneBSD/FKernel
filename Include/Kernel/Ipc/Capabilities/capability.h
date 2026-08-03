#pragma once

#include <Kernel/Ipc/Capabilities/capability_type.h>
#include <Kernel/Ipc/Capabilities/capability_rights.h>
#include <Kernel/Ipc/Capabilities/capability_meta.h>

namespace fkernel {
namespace ipc {

class Capability {
    void*          m_object{nullptr};
    CapabilityMeta m_meta{};

public:
    Capability() = default;

    Capability(void* object, CapabilityType type,
               CapabilityRights rights = CapabilityRights::All,
               const uint64_t* counter = nullptr,
               uint64_t generation = 0)
        : m_object(object)
        , m_meta{type, rights, counter, generation} {}

    void*            object() const { return m_object; }
    CapabilityType   type()   const { return m_meta.type; }
    CapabilityRights rights() const { return m_meta.rights; }

    bool is_valid() const {
        if (m_meta.type == CapabilityType::None) return false;
        if (m_meta.revoke_counter &&
            *m_meta.revoke_counter != m_meta.issued_generation)
            return false;
        return true;
    }

    bool can_send()   const { return (m_meta.rights & CapabilityRights::Send)    != CapabilityRights::None; }
    bool can_recv()   const { return (m_meta.rights & CapabilityRights::Receive) != CapabilityRights::None; }
    bool can_manage() const { return (m_meta.rights & CapabilityRights::Manage)  != CapabilityRights::None; }
    bool can_read()   const { return (m_meta.rights & CapabilityRights::Read)    != CapabilityRights::None; }
    bool can_write()  const { return (m_meta.rights & CapabilityRights::Write)   != CapabilityRights::None; }
    bool can_seek()   const { return (m_meta.rights & CapabilityRights::Seek)    != CapabilityRights::None; }
    bool can_ioctl()  const { return (m_meta.rights & CapabilityRights::Ioctl)   != CapabilityRights::None; }

    Capability with_rights(CapabilityRights r) const {
        return Capability(m_object, m_meta.type, r,
                          m_meta.revoke_counter, m_meta.issued_generation);
    }
};

} // namespace ipc
} // namespace fkernel
