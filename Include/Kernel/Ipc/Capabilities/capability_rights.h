#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

enum class CapabilityRights : uint32_t {
    None    = 0,
    Send    = 1 << 0,
    Receive = 1 << 1,
    Manage  = 1 << 2,
    Read    = 1 << 3,
    Write   = 1 << 4,
    Seek    = 1 << 5,
    Ioctl   = 1 << 6,
    All     = Send | Receive | Manage | Read | Write | Seek | Ioctl,
};

inline CapabilityRights operator|(CapabilityRights a, CapabilityRights b) {
    return static_cast<CapabilityRights>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline CapabilityRights operator&(CapabilityRights a, CapabilityRights b) {
    return static_cast<CapabilityRights>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

} // namespace ipc
} // namespace fkernel
