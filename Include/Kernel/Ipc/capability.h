#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

enum class CapabilityType : uint8_t {
    None = 0,
    Endpoint,
    Notification,
    SharedMemory,
};

enum class CapabilityRights : uint32_t {
    None    = 0,
    Send    = 1 << 0,
    Receive = 1 << 1,
    Manage  = 1 << 2,
    All     = Send | Receive | Manage,
};

inline CapabilityRights operator|(CapabilityRights a, CapabilityRights b) {
    return static_cast<CapabilityRights>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline CapabilityRights operator&(CapabilityRights a, CapabilityRights b) {
    return static_cast<CapabilityRights>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// Tracks type, rights, and revocation state for a capability.
// revoke_counter points into the target IPC object (Endpoint / Notification).
// If the object's generation no longer matches issued_generation the capability
// is considered revoked.
struct CapabilityMeta {
    CapabilityType    type{CapabilityType::None};
    CapabilityRights  rights{CapabilityRights::None};
    const uint64_t*   revoke_counter{nullptr};
    uint64_t          issued_generation{0};
};

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

    Capability with_rights(CapabilityRights r) const {
        return Capability(m_object, m_meta.type, r,
                          m_meta.revoke_counter, m_meta.issued_generation);
    }
};

} // namespace ipc
} // namespace fkernel
