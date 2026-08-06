#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {

uint64_t sys_chown(uint64_t path_ptr, uint64_t owner, uint64_t group, uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
  if (!path_ptr)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  const char* path = reinterpret_cast<const char*>(path_ptr);
  auto res = fkernel::VirtualFileSystem::the().chown(path, (uint32_t)owner, (uint32_t)group);
  if (res.is_error())
    return fkernel::return_error(res.error());
  return 0;
}

}
