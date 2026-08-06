#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

extern "C" {

uint64_t sys_getdents64(uint64_t fd, uint64_t buffer_ptr, uint64_t max_bytes, uint64_t, uint64_t,
                        uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return fkernel::return_error(fk::core::Error::PermissionDenied);

  auto description = current_task->get_file_descriptor(static_cast<int>(fd));
  if (!description)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto res = VirtualFileSystem::the().readdir(description, reinterpret_cast<uint8_t*>(buffer_ptr),
                                              max_bytes);
  if (res.is_error()) {
    fk::algorithms::kwarn("SYSCALL", "sys_getdents64: failed with error %d", (int)res.error());
    return fkernel::return_error(res.error());
  }

  return res.value();
}
}
