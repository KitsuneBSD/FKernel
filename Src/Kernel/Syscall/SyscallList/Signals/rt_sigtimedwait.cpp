#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

struct kernel_sigset {
    uint64_t sig[1];
};

struct kernel_siginfo {
    int si_signo;
    int si_errno;
    int si_code;
    int _pad[29];
};

static int dequeue_signal(Task* task, uint64_t mask) {
    uint64_t pending   = task->resources.ipc.signals.pending;
    uint64_t matchable = pending & ~task->resources.ipc.signals.blocked & mask;
    if (!matchable) return 0;
    for (int sig = 1; sig < NSIG; ++sig) {
        if (matchable & (1ULL << (sig - 1))) {
            task->resources.ipc.signals.pending &= ~(1ULL << (sig - 1));
            return sig;
        }
    }
    return 0;
}

extern "C" uint64_t sys_rt_sigtimedwait(uint64_t set_ptr, uint64_t info_ptr,
                                         uint64_t timeout_ptr, uint64_t sigsetsize,
                                         uint64_t, uint64_t,
                                         [[maybe_unused]] PtRegs* regs) {
    (void)sigsetsize;
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    // Read the signal set to wait for
    uint64_t raw_mask = 0xFFFFFFFFu;
    if (set_ptr) {
        kernel_sigset ks{};
        auto res = fkernel::memory::copy_from_user(&ks,
                                                    reinterpret_cast<const void*>(set_ptr),
                                                    sizeof(ks));
        if (res.is_error()) return (uint64_t)-14; // EFAULT
        raw_mask = ks.sig[0];
    }
    uint64_t mask = raw_mask;

    // Check for already-pending signal
    int sig = dequeue_signal(task, mask);
    if (sig > 0) {
        if (info_ptr) {
            kernel_siginfo ki{};
            ki.si_signo = sig;
            ki.si_code  = 0; // SI_USER
            fkernel::memory::copy_to_user(reinterpret_cast<void*>(info_ptr), &ki, sizeof(ki));
        }
        return (uint64_t)sig;
    }

    // No pending signal — if timeout == {0,0}, return EAGAIN immediately
    uint64_t wait_ticks = (uint64_t)-1; // default: block indefinitely
    if (timeout_ptr) {
        struct { int64_t tv_sec; int64_t tv_nsec; } ts{};
        auto res = fkernel::memory::copy_from_user(&ts,
                                                    reinterpret_cast<const void*>(timeout_ptr),
                                                    sizeof(ts));
        if (res.is_error()) return (uint64_t)-14;
        if (ts.tv_sec == 0 && ts.tv_nsec == 0)
            return (uint64_t)-11; // EAGAIN

        // Convert ns to scheduler ticks: ticks = ns * freq / 1e9
        uint32_t freq = TickManager::the().get_frequency();
        if (freq == 0) freq = 1000;
        uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
        wait_ticks = (ns / 1000000ULL) * freq / 1000ULL;
        if (wait_ticks == 0) wait_ticks = 1;
    }

    // Sleep for the timeout period; signal delivery will wake us early
    if (wait_ticks == (uint64_t)-1) SchedulerManager::the().block_current();
    if (wait_ticks != (uint64_t)-1) SchedulerManager::the().sleep_current(fk::TickCount(wait_ticks));

    // After waking, check again for the signal
    sig = dequeue_signal(task, mask);
    if (sig > 0) {
        if (info_ptr) {
            kernel_siginfo ki{};
            ki.si_signo = sig;
            ki.si_code  = 0;
            fkernel::memory::copy_to_user(reinterpret_cast<void*>(info_ptr), &ki, sizeof(ki));
        }
        return (uint64_t)sig;
    }

    return (uint64_t)-4; // EINTR / EAGAIN (timeout)
}
