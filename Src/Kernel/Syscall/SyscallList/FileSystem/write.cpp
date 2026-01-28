#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include "Kernel/Driver/SerialPort/serial_port.h"
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibC/stdio.h>
#include <LibFK/Algorithms/log.h>

extern "C" {
uint64_t sys_write(uint64_t fd, uint64_t buffer_ptr, uint64_t count, uint64_t,
                   uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *task = SchedulerManager::the().current();
  if (!task)
    return -1;

  auto description = task->get_file_descriptor(static_cast<int>(fd));
  if (!description) {
    // Fallback for kernel-level debugging if FDs are not set up,
    // but usually Init has them.
    return -1;
  }

  auto res =
      description->write(count, reinterpret_cast<const uint8_t *>(buffer_ptr));
  if (res.is_error())
    return -1;

  return res.value();
}
}
