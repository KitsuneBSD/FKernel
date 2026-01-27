#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

uint64_t sys_set_tid_address(uint64_t tidptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -1;

    task->clear_child_tid = tidptr;
    
    return task->id;
}

}
