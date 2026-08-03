#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Ipc/Signals/signal_frame.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/Logging/log.h>


// sys_kill(...) → 0 or -errno
extern "C" uint64_t sys_kill(uint64_t pid, uint64_t sig, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*) {
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
