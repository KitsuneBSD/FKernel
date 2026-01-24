#include <Kernel/Syscall/syscall.h>
#include <Kernel/Scheduler/scheduler.h>

extern "C" {

uint64_t sys_getpid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    auto* task = SchedulerManager::the().current();
    if (!task) return 1;
    return task->id;
}

uint64_t sys_gettid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    auto* task = SchedulerManager::the().current();
    if (!task) return 1;
    return task->id;
}

uint64_t sys_getppid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    return 1; // For now, every process is a child of init (1)
}

}