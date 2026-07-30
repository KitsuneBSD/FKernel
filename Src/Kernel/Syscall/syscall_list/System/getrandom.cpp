#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/chacha20.h>

extern "C" uint64_t sys_getrandom(uint64_t buf_ptr, uint64_t count, uint64_t flags,
                                   uint64_t, uint64_t, uint64_t,
                                   [[maybe_unused]] PtRegs* regs) {
  (void)flags;
  if (!buf_ptr || count == 0 || !fkernel::memory::is_user_address(buf_ptr, count))
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  uint8_t kbuf[256];
  size_t written = 0;
  while (written < (size_t)count) {
    size_t chunk = (size_t)count - written;
    if (chunk > sizeof(kbuf)) chunk = sizeof(kbuf);
    fk::algorithms::ChaCha20PRNG::the().fill_buffer(kbuf, chunk);
    auto res = fkernel::memory::copy_to_user(
        reinterpret_cast<void*>(buf_ptr + written), kbuf, chunk);
    if (res.is_error()) return -14; // EFAULT
    written += chunk;
  }
  return (uint64_t)written;
}
