#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Clock/clock_interrupt.h>

struct timeval {
    int64_t tv_sec;
    int64_t tv_usec;
};

extern "C" {

uint64_t sys_gettimeofday(uint64_t tv_ptr, [[maybe_unused]] uint64_t tz_ptr, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* tv = reinterpret_cast<struct timeval*>(tv_ptr);
    if (!tv) return fkernel::return_error(fk::core::Error::InvalidParameter);

    auto dt = ClockManager::the().datetime();
    uint64_t y = dt.year - 1970;
    uint64_t days = y * 365 + (y + 2) / 4;
    static const int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    for (int i = 0; i < dt.month - 1 && i < 12; ++i) days += days_in_month[i];
    if (dt.month > 2 && (dt.year % 4 == 0)) days++;
    days += (dt.day - 1);

    tv->tv_sec = days * 86400 + dt.hour * 3600 + dt.minute * 60 + dt.second;
    
    uint64_t ticks = TickManager::the().get_ticks();
    uint32_t freq = TickManager::the().get_frequency();
    tv->tv_usec = (freq > 0) ? (int64_t)((ticks % freq) * 1000000 / freq) : (int64_t)((ticks % 100) * 10000);

    return 0;
}

}
