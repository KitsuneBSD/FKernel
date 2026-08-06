#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Ipc/Signals/signal_frame.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_sigreturn(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs* regs) {
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
