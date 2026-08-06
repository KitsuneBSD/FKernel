#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Clock/clock_interrupt.h>

struct timespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

extern "C" {

uint64_t sys_nanosleep(uint64_t req_ptr, uint64_t rem_ptr, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* req = reinterpret_cast<const struct timespec*>(req_ptr);
    if (!req) return fkernel::return_error(fk::core::Error::InvalidParameter);

    uint64_t ms = req->tv_sec * 1000 + req->tv_nsec / 1000000;
    TickManager::the().sleep(ms);

    if (rem_ptr) {
        auto* rem = reinterpret_cast<struct timespec*>(rem_ptr);
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }

    return 0;
}

}
