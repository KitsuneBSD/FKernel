#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

enum class CapabilityType : uint8_t {
    None = 0,
    Endpoint,
    Notification,
    SharedMemory,
    FileDescription,
    Irq,  // IrqBinding — wraps a (vector, Endpoint*) binding
};

} // namespace ipc
} // namespace fkernel
