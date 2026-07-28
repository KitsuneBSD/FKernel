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

    auto pid = current->control.identity.id;

    // Send SIGKILL to all sibling threads (same PID) to terminate them
    // when they next enter the kernel. We exit last.
    for (uint64_t probe_pid = 1; probe_pid < 1024; ++probe_pid) {
        if (probe_pid == pid.value()) continue;
        auto task = SchedulerManager::the().find_task(fk::ProcessId(probe_pid));
        if (task && task->control.identity.id == pid) {
            fk::algorithms::klog("EXIT_GROUP", "Sending SIGKILL to sibling %lu", probe_pid);
            fkernel::ipc::SignalDelivery::send_signal(task.get(), SIGKILL);
        }
    }

    return sys_exit(status, 0, 0, 0, 0, 0, regs);
}
}
