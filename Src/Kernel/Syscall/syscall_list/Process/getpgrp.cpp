#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Scheduler/scheduler.h>

extern "C" {

uint64_t sys_getpgrp(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    return task->control.identity.pgid.value();
}

}
