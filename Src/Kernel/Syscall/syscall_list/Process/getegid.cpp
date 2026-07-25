#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" {

uint64_t sys_getegid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* t = SchedulerManager::the().current();
    return t ? (uint64_t)t->control.identity.egid : 0;
}

}
