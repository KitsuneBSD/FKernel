#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>

extern "C" uint64_t sys_umask(uint64_t mask, uint64_t, uint64_t, uint64_t,
                               uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    uint32_t old = task->control.identity.umask;
    task->control.identity.umask = (uint32_t)(mask & 0777);
    return (uint64_t)old;
}
