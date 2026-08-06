#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

static constexpr uint64_t PAGE_SIZE_BITS = 0xFFFULL;

// sys_madvise(addr, len, advice) → 0 or -errno
// FKernel has no page cache and no swap, so all advice is a hint that changes
// nothing. We validate the range so callers with garbage pointers get EINVAL.
extern "C" uint64_t sys_madvise(uint64_t addr_u, uint64_t len_u, uint64_t advice_u, uint64_t,
                                 uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  (void)advice_u;
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
  return 0;
}
