#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include "Kernel/Driver/SerialPort/serial_port.h"
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibC/stdio.h>
#include <LibFK/Algorithms/log.h>

struct iovec {
  void *iov_base;
  size_t iov_len;
};

extern "C" {
uint64_t sys_write(uint64_t fd, uint64_t buffer_ptr, uint64_t count, uint64_t,
                   uint64_t, uint64_t, PtRegs* regs) {
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

uint64_t sys_writev(uint64_t fd, uint64_t iov_ptr, uint64_t iovcnt, uint64_t,
                    uint64_t, uint64_t, PtRegs* regs) {
  auto *task = SchedulerManager::the().current();
  if (!task)
    return -1;

  auto description = task->get_file_descriptor(static_cast<int>(fd));
  if (!description)
    return -1;

  const iovec *iov = reinterpret_cast<const iovec *>(iov_ptr);
  size_t total_size = 0;
  for (uint64_t i = 0; i < iovcnt; ++i) {
    total_size += iov[i].iov_len;
  }

    if (total_size == 0) return 0;
    
    // Allocate temporary buffer to make the write atomic
    uint8_t* buffer = static_cast<uint8_t*>(kmalloc(total_size));
    if (!buffer) return -1;

    size_t offset = 0;
    for (uint64_t i = 0; i < iovcnt; ++i) {
        memcpy(buffer + offset, iov[i].iov_base, iov[i].iov_len);
        offset += iov[i].iov_len;
    }

    auto res = description->write(total_size, buffer);

  kfree(buffer);

  if (res.is_error())
    return fkernel::return_error(res.error());

  // fk::algorithms::klog("SYSCALL", "sys_writev: wrote %zu bytes to FD %lu",
  // res.value(), fd);

  return res.value();
}
}
