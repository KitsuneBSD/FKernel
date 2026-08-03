#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Ipc/Signals/signal_frame.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/Logging/log.h>


// sys_sigprocmask(...) → 0 or -errno
extern "C" uint64_t sys_sigprocmask(uint64_t how, uint64_t set_ptr, uint64_t oldset_ptr, uint64_t, uint64_t, uint64_t, PtRegs*) {
    if (set_ptr && !fkernel::memory::is_user_address(set_ptr, sizeof(uint64_t))) return -14;
    if (oldset_ptr && !fkernel::memory::is_user_address(oldset_ptr, sizeof(uint64_t))) return -14;

    auto* current = SchedulerManager::the().current();

    fk::synchronization::ScopedLock lock(current->lock);

    if (oldset_ptr) {
        uint64_t blocked = current->resources.ipc.signals.blocked;
        auto res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(oldset_ptr), &blocked, sizeof(uint64_t));
        if (res.is_error()) return -14;
    }

    if (set_ptr) {
        uint64_t k_set = 0;
        auto res = fkernel::memory::copy_from_user(&k_set, reinterpret_cast<const void*>(set_ptr), sizeof(uint64_t));
        if (res.is_error()) return -14;
        k_set &= ~((1ULL << SIGKILL) | (1ULL << SIGSTOP));

        switch (how) {
            case 0: // SIG_BLOCK
                current->resources.ipc.signals.blocked |= k_set;
                break;
            case 1: // SIG_UNBLOCK
                current->resources.ipc.signals.blocked &= ~k_set;
                break;
            case 2: // SIG_SETMASK
                current->resources.ipc.signals.blocked = k_set;
                break;
            default:
                return -22; // EINVAL
        }
    }

    return 0;
}
