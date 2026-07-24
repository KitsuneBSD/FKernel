#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {

uint64_t sys_rt_sigsuspend(uint64_t mask_ptr, uint64_t sigsetsize, uint64_t, uint64_t,
                           uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    (void)sigsetsize;

    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    // Save current signal mask
    uint32_t old_blocked = task->resources.ipc.signals.blocked;

    // Apply new mask if provided (SIGKILL and SIGSTOP cannot be blocked)
    if (mask_ptr) {
        uint64_t new_mask_raw = 0;
        auto res = fkernel::memory::copy_from_user(&new_mask_raw,
                                                    reinterpret_cast<const void*>(mask_ptr),
                                                    sizeof(uint64_t));
        if (res.is_error()) return (uint64_t)-14; // EFAULT

        uint32_t new_mask = (uint32_t)new_mask_raw;
        // Cannot block SIGKILL (9) or SIGSTOP (19)
        new_mask &= ~((1u << (SIGKILL - 1)) | (1u << (SIGSTOP - 1)));
        task->resources.ipc.signals.blocked = new_mask;
    }

    // Block until a signal is delivered (scheduler will unblock on signal)
    SchedulerManager::the().block_current();
    SchedulerManager::the().schedule();

    // Restore original mask before returning
    task->resources.ipc.signals.blocked = old_blocked;

    // sigsuspend always returns -EINTR
    return (uint64_t)-4; // EINTR
}
}
