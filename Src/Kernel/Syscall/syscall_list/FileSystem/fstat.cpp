#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Posix/sys/stat.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/vector.h>

extern "C" {

uint64_t sys_fstat(uint64_t fd, uint64_t statbuf_ptr, uint64_t, uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task)
    return fkernel::return_error(fk::core::Error::PermissionDenied);

  auto description = task->get_file_descriptor(static_cast<int>(fd));
  if (!description)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto* buf = reinterpret_cast<struct stat*>(statbuf_ptr);
  if (!buf)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto node = description->node();
  fk::memory::set(buf, 0, sizeof(struct stat));
  buf->st_dev = 1;
  buf->st_ino = node->inode();
  buf->st_size = node->size();
  buf->st_blksize = 4096;
  buf->st_blocks = (buf->st_size + 511) / 512;
  buf->st_uid = node->node_uid();
  buf->st_gid = node->node_gid();

  uint64_t now = TickManager::the().get_ticks();
  buf->st_atime = now;
  buf->st_mtime = now;
  buf->st_ctime = now;

  buf->st_mode = node->node_mode();
  if (node->is_directory()) {
    buf->st_nlink = 2;
    if (buf->st_size == 0)
      buf->st_size = 4096;
  } else {
    buf->st_nlink = 1;
  }

  return 0;
}
}
