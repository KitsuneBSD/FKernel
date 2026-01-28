#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Clock/clock_interrupt.h>
#include <LibFK/Algorithms/log.h>

struct timespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

extern "C" {

uint64_t sys_clock_gettime([[maybe_unused]] uint64_t clk_id, uint64_t tp_ptr, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* tp = reinterpret_cast<struct timespec*>(tp_ptr);
    if (!tp) return fkernel::return_error(fk::core::Error::InvalidParameter);

    auto dt = ClockManager::the().datetime();
    
    // Convert DateTime to Unix epoch (simplified but better)
    uint64_t y = dt.year - 1970;
    uint64_t days = y * 365 + (y + 2) / 4; // Leap years
    static const int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    for (int i = 0; i < dt.month - 1 && i < 12; ++i) days += days_in_month[i];
    if (dt.month > 2 && (dt.year % 4 == 0)) days++;
    days += (dt.day - 1);

    tp->tv_sec = days * 86400 + dt.hour * 3600 + dt.minute * 60 + dt.second;
    
    uint64_t ticks = TickManager::the().get_ticks();
    tp->tv_nsec = (ticks % 100) * 10000000; // 100Hz ticks -> 10ms increments

    fk::algorithms::klog("SYSCALL", "clock_gettime: %lu seconds", tp->tv_sec);

    return 0;
}

}
