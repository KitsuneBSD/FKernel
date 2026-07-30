#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Ipc/signal_frame.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>


extern "C" {

uint64_t sys_kill(uint64_t pid, uint64_t sig, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*) {
    int32_t kpid = static_cast<int32_t>(static_cast<uint32_t>(pid));

    if (kpid <= 0) {
        int pgid = (kpid == 0) ? SchedulerManager::the().current()->control.identity.pgid.value()
                               : -kpid;
        SchedulerManager::the().send_signal_to_pgrp(pgid, (int)sig);
        return 0;
    }

    fk::algorithms::kdebug("SIGNAL", "kill: sending signal %d to TGID %ld", (int)sig, (int64_t)kpid);

    siginfo_t info{};
    info.si_signo = (int)sig;
    info.si_code  = SI_USER;
    info.si_pid   = SchedulerManager::the().current()->control.identity.id.value();
    info.si_uid   = SchedulerManager::the().current()->control.identity.uid;
    // Deliver to the thread group (any non-blocking thread, prefer leader)
    fkernel::ipc::SignalDelivery::deliver_to_group((int)sig,
                                                    fk::ProcessId(static_cast<uint64_t>(kpid)),
                                                    &info);
    return 0;
}

uint64_t sys_sigaction(uint64_t sig, uint64_t act_ptr, uint64_t oldact_ptr, uint64_t, uint64_t, uint64_t, PtRegs*) {
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

uint64_t sys_sigprocmask(uint64_t how, uint64_t set_ptr, uint64_t oldset_ptr, uint64_t, uint64_t, uint64_t, PtRegs*) {
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

uint64_t sys_sigreturn(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs* regs) {
    if (!regs) return -1;

    uint64_t frame_ptr = regs->rsp;
    static constexpr uint64_t USERSPACE_MAX = 0x0000800000000000ULL;
    if (frame_ptr == 0 || frame_ptr >= USERSPACE_MAX || (frame_ptr & 7)) return -1;

    uint64_t saved_mask = 0;
    auto mask_res = fkernel::memory::copy_from_user(&saved_mask,
                                                    reinterpret_cast<const void*>(frame_ptr),
                                                    sizeof(uint64_t));
    if (mask_res.is_error()) return -1;

    static constexpr size_t SIGINFO_OFFSET = sizeof(uint64_t) + sizeof(siginfo_t);
    PtRegs saved_regs{};
    auto res = fkernel::memory::copy_from_user(&saved_regs,
                                               reinterpret_cast<const void*>(frame_ptr + SIGINFO_OFFSET),
                                               sizeof(PtRegs));
    if (res.is_error()) return -1;

    auto* current = SchedulerManager::the().current();
    if (current) {
        fk::synchronization::ScopedLock lock(current->lock);
        current->resources.ipc.signals.blocked = saved_mask;
        current->resources.ipc.signals.blocked &= ~((1ULL << SIGKILL) | (1ULL << SIGSTOP));
    }

    *regs = saved_regs;

    current_cpu_block().saved_rip    = saved_regs.rip;
    current_cpu_block().saved_rflags = saved_regs.rflags;
    current_cpu_block().user_rsp     = saved_regs.rsp;

    return regs->rax;
}

}
