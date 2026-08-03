#pragma once

#include <Kernel/Ipc/Capabilities/capability_type.h>
#include <Kernel/Ipc/Capabilities/capability_rights.h>

namespace fkernel {
namespace ipc {

struct CapabilityMeta {
    CapabilityType    type{CapabilityType::None};
    CapabilityRights  rights{CapabilityRights::None};
    const uint64_t*   revoke_counter{nullptr};
    uint64_t          issued_generation{0};
};

} // namespace ipc
} // namespace fkernel
