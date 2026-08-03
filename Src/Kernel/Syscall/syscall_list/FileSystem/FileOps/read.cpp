#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/error.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>
#include <LibFK/Types/types.h>

extern "C" {
uint64_t sys_read(uint64_t fd_u64, uint64_t buffer_ptr, uint64_t size,
                  uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {

  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (size == 0) return 0;

  if (!fkernel::memory::is_user_address(buffer_ptr, size))
    return -14; // EFAULT

  auto desc = current_task->get_file_descriptor(static_cast<int>(fd_u64));
  if (!desc)
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  uint8_t* kbuf = static_cast<uint8_t*>(kmalloc(size));
  if (!kbuf) return -12; // ENOMEM

  auto result = desc->read(size, kbuf);
  if (result.is_error()) {
    kfree(kbuf);
    return -static_cast<int>(result.error());
  }

  size_t n = result.value();
  auto copy_res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(buffer_ptr), kbuf, n);
  kfree(kbuf);
  if (copy_res.is_error()) return -14; // EFAULT
  return n;
}
}