#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

// sys_utimes(...) → 0 or -errno
extern "C" uint64_t sys_utimes(uint64_t path_ptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
  const char* path = reinterpret_cast<const char*>(path_ptr);
  if (!path)
    return return_error(fk::core::Error::InvalidParameter);
  auto res = VirtualFileSystem::the().resolve_path(path);
  if (res.is_error())
    return return_error(res.error());
  return 0;
}
