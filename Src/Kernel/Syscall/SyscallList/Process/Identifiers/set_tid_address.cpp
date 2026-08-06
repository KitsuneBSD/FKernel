#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Scheduler/Core/scheduler.h>

extern "C" {

uint64_t sys_set_tid_address(uint64_t tidptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -1;

    task->control.lifecycle.clear_child_tid = tidptr;
    
    return task->control.identity.id.value();
}

}