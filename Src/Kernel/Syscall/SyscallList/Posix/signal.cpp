#include <Kernel/Syscall/syscall.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Ipc/signal_frame.h>
#include <Kernel/Posix/signal_defs.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

uint64_t sys_kill(uint64_t pid, uint64_t sig, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*) {
    auto* target = SchedulerManager::the().find_task(fk::ProcessId(pid));
    if (!target) return -3; // ESRCH

    fkernel::ipc::SignalDelivery::send_signal(target, (int)sig);
    return 0;
}

uint64_t sys_sigaction(uint64_t sig, uint64_t act_ptr, uint64_t oldact_ptr, uint64_t, uint64_t, uint64_t, PtRegs*) {
    if (sig == 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP) return -22; // EINVAL

    auto* current = SchedulerManager::the().current();
    auto* new_act = reinterpret_cast<const struct sigaction*>(act_ptr);
    auto* old_act = reinterpret_cast<struct sigaction*>(oldact_ptr);

    fk::synchronization::ScopedLock lock(current->lock);

    if (old_act) {
        *old_act = current->resources.ipc.signals.actions[sig];
    }

    if (new_act) {
        current->resources.ipc.signals.actions[sig] = *new_act;
    }

    return 0;
}

uint64_t sys_sigprocmask(uint64_t how, uint64_t set_ptr, uint64_t oldset_ptr, uint64_t, uint64_t, uint64_t, PtRegs*) {
    auto* current = SchedulerManager::the().current();
    auto* set = reinterpret_cast<const uint32_t*>(set_ptr);
    auto* oldset = reinterpret_cast<uint32_t*>(oldset_ptr);

    fk::synchronization::ScopedLock lock(current->lock);

    if (oldset) {
        *oldset = current->resources.ipc.signals.blocked;
    }

    if (set) {
        uint32_t new_mask = *set;
        // Cannot block SIGKILL or SIGSTOP
        new_mask &= ~((1 << SIGKILL) | (1 << SIGSTOP));

        switch (how) {
            case 0: // SIG_BLOCK
                current->resources.ipc.signals.blocked |= new_mask;
                break;
            case 1: // SIG_UNBLOCK
                current->resources.ipc.signals.blocked &= ~new_mask;
                break;
            case 2: // SIG_SETMASK
                current->resources.ipc.signals.blocked = new_mask;
                break;
            default:
                return -22; // EINVAL
        }
    }

    return 0;
}

uint64_t sys_sigreturn(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs* regs) {
    if (!regs) return -1;

    // After handler RET, rsp points to saved_regs (pretcode already popped).
    // Read the KernelSignalFrame that was placed there by handle_pending_signals.
    uint64_t frame_ptr = regs->rsp;
    static constexpr uint64_t USERSPACE_MAX = 0x0000800000000000ULL;
    if (frame_ptr >= USERSPACE_MAX || frame_ptr == 0) return -1;

    const auto* saved = reinterpret_cast<const PtRegs*>(frame_ptr);
    *regs = *saved;
    // rax = 0 means the syscall "returns 0" — overwritten by restoring saved_regs
    return regs->rax;
}

}
