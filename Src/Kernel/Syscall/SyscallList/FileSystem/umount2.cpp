#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {
uint64_t sys_umount2(uint64_t target_ptr, uint64_t /*flags*/, [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task) return fkernel::return_error(fk::core::Error::PermissionDenied);

  const char* target = reinterpret_cast<const char*>(target_ptr);
  if (!target) return fkernel::return_error(fk::core::Error::InvalidParameter);

  fk::algorithms::klog("sys_umount2", "target=%s", target);

  auto res = fkernel::VirtualFileSystem::the().unmount(target);
  if (res.is_error()) {
    fk::algorithms::kwarn("sys_umount2", "Failed to unmount %s", target);
    return fkernel::return_error(res.error());
  }
  return 0;
}
} // extern "C"
