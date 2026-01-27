#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

extern "C" {
uint64_t sys_exit(uint64_t status, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* current = SchedulerManager::the().current();
    fk::algorithms::klog("SYSCALL", "Process %lu exiting with status %lu", current->id, status);
    
    current->terminated = true;
    current->exit_status = (int)status;

    auto* parent = SchedulerManager::the().find_task(current->ppid);
    if (parent) {
        if (parent->vfork_waiting && current->vfork_parent_id == parent->id) {
            parent->vfork_waiting = false;
        }
        fk::algorithms::klog("SYSCALL", "Waking parent PID %lu", parent->id);
        SchedulerManager::the().wake_task(parent);
    }

    // Explicitly yield control now that we are done
    SchedulerManager::the().zombify_current();
    SchedulerManager::the().schedule();

    // Should never reach here
    while(true) { asm volatile("hlt"); }
    return 0; 
}

uint64_t sys_exit_group(uint64_t status, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, [[maybe_unused]] PtRegs* regs) {
    return sys_exit(status, a1, a2, a3, a4, a5);
}
}
