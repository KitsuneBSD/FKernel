#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

static constexpr uint64_t PAGE_SIZE_BITS = 0xFFFULL;

// sys_mlock(addr, len) → 0 or -errno
// FKernel has no swap and never evicts user pages, so "locking" is a no-op.
// We still validate that the range is a valid, fully-mapped user range so
// callers observe the same error behavior as a paging kernel.
extern "C" uint64_t sys_mlock(uint64_t addr_u, uint64_t len_u, uint64_t, uint64_t, uint64_t,
                   uint64_t, [[maybe_unused]] PtRegs* regs) {
  if (len_u == 0)
    return 0;

  uintptr_t start = addr_u & ~PAGE_SIZE_BITS;
  if (addr_u + len_u < addr_u)
    return return_error(fk::core::Error::InvalidParameter);
  uintptr_t end = (addr_u + len_u + PAGE_SIZE_BITS) & ~PAGE_SIZE_BITS;
  if (end < start)
    return return_error(fk::core::Error::InvalidParameter);
  if (!memory::is_user_address(start, end - start))
    return return_error(fk::core::Error::InvalidParameter);

  for (uintptr_t page = start; page < end; page += 0x1000) {
    if (MemoryManager::the().translate(page) == 0)
      return return_error(fk::core::Error::OutOfMemory); // ENOMEM: page not mapped
  }
  return 0;
}
