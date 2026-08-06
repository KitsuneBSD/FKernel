#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {

uint64_t sys_mkfifo(uint64_t path_ptr, uint64_t mode,
                    uint64_t, uint64_t, uint64_t, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
  const char* path = reinterpret_cast<const char*>(path_ptr);
  if (!path) return (uint64_t)-14;

  auto result = fkernel::VirtualFileSystem::the().mkfifo(path, (int)mode);
  if (result.is_error())
    return fkernel::return_error(result.error());
  return 0;
}

}
