#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" {
uint64_t sys_yield(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    // Force a reschedule
    SchedulerManager::the().set_need_resched(true);
    return 0;
}
}
