#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Syscall/posix_timer.h>
#include <LibFK/Utilities/memory.h>

static constexpr uint32_t TICKS_PER_SEC = 1000;

// sys_timer_settime(...) → 0 or -errno
extern "C" uint64_t sys_timer_settime(uint64_t timerid, uint64_t flags, uint64_t new_value_ptr,
                           uint64_t old_value_ptr, uint64_t, uint64_t,
                           [[maybe_unused]] PtRegs* regs) {
  (void)flags;
  int id = (int)timerid;
  if (id < 0 || id >= MAX_POSIX_TIMERS || !s_timers[id].used)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto& timer = s_timers[id];

  // Report old value
  if (old_value_ptr) {
    // itimerspec: { it_interval (2x uint64), it_value (2x uint64) } = 32 bytes
    auto* ov = reinterpret_cast<uint64_t*>(old_value_ptr);
    if (timer.expiry_ticks > 0) {
      uint64_t remaining_us = (timer.expiry_ticks * 1000000) / TICKS_PER_SEC;
      ov[2] = remaining_us / 1000000;        // it_value.tv_sec
      ov[3] = remaining_us % 1000000;        // it_value.tv_nsec (using tv_usec slot)
    } else {
      ov[2] = 0; ov[3] = 0;
    }
    uint64_t int_us = (timer.interval_ticks * 1000000) / TICKS_PER_SEC;
    ov[0] = int_us / 1000000;               // it_interval.tv_sec
    ov[1] = int_us % 1000000;               // it_interval.tv_nsec
  }

  // Set new value
  if (new_value_ptr) {
    auto* nv = reinterpret_cast<const uint64_t*>(new_value_ptr);
    uint64_t value_sec = nv[2];
    uint64_t value_nsec = nv[3];
    uint64_t int_sec = nv[0];
    uint64_t int_nsec = nv[1];

    uint64_t value_us = value_sec * 1000000 + value_nsec / 1000;
    uint64_t int_us = int_sec * 1000000 + int_nsec / 1000;

    timer.expiry_ticks = (value_us * TICKS_PER_SEC) / 1000000;
    timer.interval_ticks = (int_us * TICKS_PER_SEC) / 1000000;
  }

  return 0;
}
