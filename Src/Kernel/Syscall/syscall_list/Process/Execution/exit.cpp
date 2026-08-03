#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/Logging/log.h>

extern "C" {
uint64_t sys_exit(uint64_t status, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    fk::algorithms::klog("EXIT", "exit(%d)", static_cast<int>(status));
    SchedulerManager::the().terminate_current(static_cast<int>(status));
    
    // Should never reach here
    arch_halt_loop();
    return 0; 
}
}
