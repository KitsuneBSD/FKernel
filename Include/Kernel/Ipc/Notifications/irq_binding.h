#pragma once

#include <Kernel/Ipc/Endpoints/endpoint.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

struct InterruptFrame;

namespace fkernel {
namespace ipc {

// Binds an IDT vector (32–255) to an Endpoint: on interrupt, signals the endpoint.
// Userspace drivers call sys_bind_irq(vector, ep_handle) to wire an IRQ.
class IrqBinding {
    static Endpoint* s_endpoints[256]; // zero-initialized in BSS

public:
    // Returns Error::InvalidParameter if vector < 32, Error::AlreadyExists if bound.
    static fk::core::Result<void, fk::core::Error> install(uint8_t vector, Endpoint* ep);
    static void remove(uint8_t vector);

    // ISR entry point registered with InterruptController.
    static void on_irq(uint8_t vector, InterruptFrame* frame);
};

} // namespace ipc
} // namespace fkernel
