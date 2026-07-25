#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>

// XOR-shift PRNG seeded from ticks — same as /dev/urandom node
static uint64_t xorshift64(uint64_t& state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

extern "C" uint64_t sys_getrandom(uint64_t buf_ptr, uint64_t count, uint64_t flags,
                                   uint64_t, uint64_t, uint64_t,
                                   [[maybe_unused]] PtRegs* regs) {
  (void)flags;
  if (!buf_ptr || count == 0)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  auto* dst = reinterpret_cast<uint8_t*>(buf_ptr);
  static uint64_t state = 0;
  if (!state)
    state = TickManager::the().get_ticks() ^ 0xdeadbeefcafe1234ULL;
  size_t written = 0;
  while (written < (size_t)count) {
    uint64_t val = xorshift64(state);
    size_t chunk = (size_t)count - written;
    if (chunk > 8) chunk = 8;
    for (size_t i = 0; i < chunk; ++i)
      dst[written + i] = (uint8_t)(val >> (i * 8));
    written += chunk;
  }
  return (uint64_t)written;
}
