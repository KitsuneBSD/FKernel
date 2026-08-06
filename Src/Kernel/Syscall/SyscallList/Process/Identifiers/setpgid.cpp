#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

extern "C" uint64_t sys_setpgid(uint64_t pid_arg, uint64_t pgid_arg, uint64_t,
                                 uint64_t, uint64_t, uint64_t,
                                 [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    // pid=0 means self; pgid=0 means use pid as pgid
    uint64_t pid  = (pid_arg  == 0) ? task->control.identity.id.value()   : pid_arg;
    uint64_t pgid = (pgid_arg == 0) ? pid : pgid_arg;

    // Only support setting own pgid for now
    if (pid != task->control.identity.id.value())
        return fkernel::return_error(fk::core::Error::PermissionDenied);

    task->control.identity.pgid = fk::ProcessId(pgid);
    return 0;
}
