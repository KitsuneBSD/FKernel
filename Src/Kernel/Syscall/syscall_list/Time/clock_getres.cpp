#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

struct timespec {
  int64_t tv_sec;
  int64_t tv_nsec;
};

extern "C" uint64_t sys_clock_getres([[maybe_unused]] uint64_t clk_id, uint64_t res_ptr,
                                      uint64_t, uint64_t, uint64_t, uint64_t,
                                      [[maybe_unused]] PtRegs* regs) {
  if (!res_ptr) return 0;
  auto* res = reinterpret_cast<timespec*>(res_ptr);
  uint32_t freq = TickManager::the().get_frequency();
  res->tv_sec = 0;
  res->tv_nsec = (freq > 0) ? (int64_t)(1000000000ULL / freq) : 1000000;
  return 0;
}
