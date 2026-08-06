#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>

#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {
uint64_t sys_write(uint64_t fd, uint64_t buffer_ptr, uint64_t count, uint64_t,
                   uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *task = SchedulerManager::the().current();
  if (!task)
    return -1;

  if (count == 0) return 0;

  if (!fkernel::memory::is_user_address(buffer_ptr, count))
    return -14; // EFAULT

  auto description = task->get_file_descriptor(static_cast<int>(fd));
  if (!description)
    return -1;

  uint8_t* kbuf = static_cast<uint8_t*>(kmalloc(count));
  if (!kbuf) return -12; // ENOMEM

  auto copy_res = fkernel::memory::copy_from_user(kbuf, reinterpret_cast<const void*>(buffer_ptr), count);
  if (copy_res.is_error()) {
    kfree(kbuf);
    return -14; // EFAULT
  }

  auto res = description->write(count, kbuf);
  kfree(kbuf);
  if (res.is_error()) {
    if (res.error() == fk::core::Error::BrokenPipe) {
      fkernel::ipc::SignalDelivery::send_signal(task, 13); // SIGPIPE
      return -32; // EPIPE
    }
    return -1;
  }

  return res.value();
}
}
