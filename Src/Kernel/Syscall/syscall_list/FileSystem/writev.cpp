#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>

struct iovec {
  void *iov_base;
  size_t iov_len;
};

static constexpr size_t IOV_MAX = 1024;
static constexpr size_t WRITEV_MAX_BYTES = 0x7ffff000; // 2GB - 1 page

extern "C" {
uint64_t sys_writev(uint64_t fd, uint64_t iov_ptr, uint64_t iovcnt, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *task = SchedulerManager::the().current();
  if (!task) return -1;

  if (iovcnt == 0) return 0;
  if (iovcnt > IOV_MAX) return -22; // EINVAL
  if (!fkernel::memory::is_user_address(iov_ptr, iovcnt * sizeof(iovec))) return -14;

  auto description = task->get_file_descriptor(static_cast<int>(fd));
  if (!description) return -9; // EBADF

  iovec* k_iov = static_cast<iovec*>(kmalloc(iovcnt * sizeof(iovec)));
  if (!k_iov) return -12; // ENOMEM

  auto iov_copy = fkernel::memory::copy_from_user(k_iov, reinterpret_cast<const void*>(iov_ptr),
                                                   iovcnt * sizeof(iovec));
  if (iov_copy.is_error()) { kfree(k_iov); return -14; }

  size_t total_size = 0;
  for (uint64_t i = 0; i < iovcnt; ++i) {
    if (k_iov[i].iov_len > WRITEV_MAX_BYTES - total_size) { kfree(k_iov); return -22; }
    total_size += k_iov[i].iov_len;
  }

  if (total_size == 0) { kfree(k_iov); return 0; }

  uint8_t* buffer = static_cast<uint8_t*>(kmalloc(total_size));
  if (!buffer) { kfree(k_iov); return -12; }

  size_t offset = 0;
  for (uint64_t i = 0; i < iovcnt; ++i) {
    if (k_iov[i].iov_len == 0) continue;
    if (!fkernel::memory::is_user_address(reinterpret_cast<uint64_t>(k_iov[i].iov_base), k_iov[i].iov_len)) {
      kfree(k_iov); kfree(buffer); return -14;
    }
    auto copy = fkernel::memory::copy_from_user(buffer + offset, k_iov[i].iov_base, k_iov[i].iov_len);
    if (copy.is_error()) { kfree(k_iov); kfree(buffer); return -14; }
    offset += k_iov[i].iov_len;
  }

  kfree(k_iov);
  auto res = description->write(total_size, buffer);
  kfree(buffer);

  if (res.is_error()) return fkernel::return_error(res.error());
  return res.value();
}
}
