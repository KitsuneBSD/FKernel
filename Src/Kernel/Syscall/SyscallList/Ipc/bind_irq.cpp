#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/Capabilities/capability.h>
#include <Kernel/Ipc/Capabilities/capability_type.h>
#include <Kernel/Ipc/Capabilities/cspace.h>
#include <Kernel/Ipc/Endpoints/endpoint.h>
#include <Kernel/Ipc/Notifications/irq_binding.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

// sys_bind_irq(vector, ep_handle) → 0 or -errno
// Registers endpoint ep_handle to receive a signal on hardware IRQ `vector`.
// vector must be in [32, 255] (hardware IRQ range; vectors 0-31 are exceptions).
extern "C" uint64_t sys_bind_irq(uint64_t vector,
                                  uint64_t ep_handle,
                                  [[maybe_unused]] uint64_t a2,
                                  [[maybe_unused]] uint64_t a3,
                                  [[maybe_unused]] uint64_t a4,
                                  [[maybe_unused]] uint64_t a5,
                                  [[maybe_unused]] PtRegs* regs) {
    using namespace fkernel::ipc;

    if (vector < 32 || vector > 255)
        return -static_cast<uint64_t>(fk::core::Error::InvalidParameter);

    auto* task = SchedulerManager::the().current();
    if (!task || !task->ipc().cspace)
        return -static_cast<uint64_t>(fk::core::Error::InvalidParameter);

    auto cap = task->ipc().cspace->get(static_cast<uint32_t>(ep_handle));
    if (!cap.is_valid() || cap.type() != CapabilityType::Endpoint)
        return -static_cast<uint64_t>(fk::core::Error::InvalidParameter);

    auto* ep = static_cast<Endpoint*>(cap.object());
    auto res = IrqBinding::install(static_cast<uint8_t>(vector), ep);
    if (res.is_error()) return -static_cast<uint64_t>(res.error());

    // Install a capability representing this IRQ binding in the caller's CSpace.
    uint32_t irq_handle = task->ipc().cspace->install(
        Capability(ep, CapabilityType::Irq, CapabilityRights::All));
    return static_cast<uint64_t>(irq_handle);
}
