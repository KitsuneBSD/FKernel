#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/Capabilities/capability.h>
#include <Kernel/Ipc/Capabilities/capability_type.h>
#include <Kernel/Ipc/Capabilities/cspace.h>
#include <Kernel/Ipc/Endpoints/endpoint.h>
#include <Kernel/Ipc/Notifications/irq_binding.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

// sys_unbind_irq(vector) → 0 or -errno
extern "C" uint64_t sys_unbind_irq(uint64_t vector,
                                    [[maybe_unused]] uint64_t a1,
                                    [[maybe_unused]] uint64_t a2,
                                    [[maybe_unused]] uint64_t a3,
                                    [[maybe_unused]] uint64_t a4,
                                    [[maybe_unused]] uint64_t a5,
                                    [[maybe_unused]] PtRegs* regs) {
    if (vector < 32 || vector > 255)
        return -static_cast<uint64_t>(fk::core::Error::InvalidParameter);
    fkernel::ipc::IrqBinding::remove(static_cast<uint8_t>(vector));
    return 0;
}
