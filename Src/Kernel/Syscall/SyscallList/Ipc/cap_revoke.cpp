#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/Capabilities/capability.h>
#include <Kernel/Ipc/Capabilities/cspace.h>
#include <Kernel/Ipc/Endpoints/endpoint.h>
#include <Kernel/Ipc/Notifications/notification.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_cap_revoke(uint64_t handle,
                                    [[maybe_unused]] uint64_t a1,
                                    [[maybe_unused]] uint64_t a2,
                                    [[maybe_unused]] uint64_t a3,
                                    [[maybe_unused]] uint64_t a4,
                                    [[maybe_unused]] uint64_t a5,
                                    [[maybe_unused]] PtRegs* regs) {
    using namespace fkernel::ipc;
    auto* task = SchedulerManager::the().current();
    if (!task || !task->ipc().cspace)
        return static_cast<uint64_t>(-1);

    auto cap = task->ipc().cspace->get(static_cast<uint32_t>(handle));
    if (!cap.is_valid())
        return static_cast<uint64_t>(-1);
    if (!cap.can_manage())
        return -static_cast<uint64_t>(fk::core::Error::PermissionDenied);

    if (cap.type() == CapabilityType::Endpoint) {
        auto* ep = static_cast<Endpoint*>(cap.object());
        ep->revoke();
    } else if (cap.type() == CapabilityType::Notification) {
        auto* notif = static_cast<Notification*>(cap.object());
        notif->revoke();
    }

    task->ipc().cspace->remove(static_cast<uint32_t>(handle));
    return 0;
}
