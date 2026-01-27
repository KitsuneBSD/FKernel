#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_getpgid(uint64_t pid, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs* regs) {
    if (pid == 0) {
        auto* task = SchedulerManager::the().current();
        return task ? task->id : 0; // Simplified: return task ID as PGID
    }
    auto* task = SchedulerManager::the().find_task(pid);
    if (!task) return -1;
    return task->id; // Simplified
}
