#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Ipc/Signals/signal_frame.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/Logging/log.h>


// sys_sigaction(...) → 0 or -errno
extern "C" uint64_t sys_sigaction(uint64_t sig, uint64_t act_ptr, uint64_t oldact_ptr, uint64_t, uint64_t, uint64_t, PtRegs*) {
    if (sig == 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP) return -22; // EINVAL
    if (act_ptr && !fkernel::memory::is_user_address(act_ptr, sizeof(struct sigaction))) return -14;
    if (oldact_ptr && !fkernel::memory::is_user_address(oldact_ptr, sizeof(struct sigaction))) return -14;

    auto* current = SchedulerManager::the().current();

    fk::synchronization::ScopedLock lock(current->lock);

    if (oldact_ptr) {
        auto res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(oldact_ptr),
                                                  &current->resources.ipc.signals.actions[sig],
                                                  sizeof(struct sigaction));
        if (res.is_error()) return -14;
    }

    if (act_ptr) {
        struct sigaction k_act{};
        auto res = fkernel::memory::copy_from_user(&k_act, reinterpret_cast<const void*>(act_ptr),
                                                    sizeof(struct sigaction));
        if (res.is_error()) return -14;
        current->resources.ipc.signals.actions[sig] = k_act;
    }

    return 0;
}
