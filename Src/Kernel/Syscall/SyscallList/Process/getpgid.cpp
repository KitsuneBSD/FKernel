#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_getpgid(uint64_t pid, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().find_task(fk::ProcessId(pid));
    if (!task) return -1;
    return task->identity.id.value(); // Simplified
}
