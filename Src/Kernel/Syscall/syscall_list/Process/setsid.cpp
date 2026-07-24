#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" uint64_t sys_setsid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    // POSIX: fail if process is already a process group leader
    if (task->control.identity.id == task->control.identity.pgid)
        return fkernel::return_error(fk::core::Error::PermissionDenied);

    fk::ProcessId new_sid = task->control.identity.id;
    task->control.identity.sid              = new_sid;
    task->control.identity.pgid             = new_sid;
    task->control.identity.is_session_leader = true;
    return new_sid.value();
}
