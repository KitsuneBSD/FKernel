#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

extern "C" {
uint64_t sys_exit(uint64_t status, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs* regs);

uint64_t sys_exit_group(uint64_t status, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs* regs) {
    auto* current = SchedulerManager::the().current();
    if (!current) return 0;

    auto self_pid = current->control.identity.id;
    auto tgid = current->control.identity.tgid;

    // Kill all threads in the same thread group (same tgid), except ourselves.
    uint64_t max_pid = SchedulerManager::the().last_pid().value();
    for (uint64_t probe = 1; probe <= max_pid; ++probe) {
        if (probe == self_pid.value()) continue;
        auto task = SchedulerManager::the().find_task(fk::ProcessId(probe));
        if (!task) continue;
        if (task->control.identity.tgid != tgid) continue;
        fk::algorithms::klog("EXIT_GROUP", "Sending SIGKILL to thread %lu (tgid=%lu)", probe, tgid.value());
        fkernel::ipc::SignalDelivery::send_signal(task.get(), SIGKILL);
    }

    return sys_exit(status, 0, 0, 0, 0, 0, regs);
}
}
