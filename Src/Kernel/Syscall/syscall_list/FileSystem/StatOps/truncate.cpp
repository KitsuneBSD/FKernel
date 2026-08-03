#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

// sys_truncate(...) → 0 or -errno
extern "C" uint64_t sys_truncate(uint64_t path_u64, uint64_t length, uint64_t,
                      uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  const char* path = reinterpret_cast<const char*>(path_u64);
  if (!path)
    return -static_cast<int>(fk::core::Error::InvalidParameter);

  auto res = fkernel::VirtualFileSystem::the().truncate(path, length);
  if (res.is_error())
    return -static_cast<int>(res.error());

  return 0;
}
