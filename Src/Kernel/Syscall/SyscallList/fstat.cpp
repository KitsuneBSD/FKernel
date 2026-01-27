#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Posix/sys/stat.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/vector.h>
#include <LibC/string.h>

extern "C" {

uint64_t sys_fstat(uint64_t fd, uint64_t statbuf_ptr, uint64_t, uint64_t,
                   uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);

  auto description = task->get_file_descriptor(static_cast<int>(fd));
  if (!description) return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto* buf = reinterpret_cast<struct stat*>(statbuf_ptr);
  if (!buf) return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto node = description->node();
  memset(buf, 0, sizeof(struct stat));
  buf->st_dev = 1;
  buf->st_size = node->size();
  buf->st_blksize = 4096;
  buf->st_blocks = (buf->st_size + 511) / 512;
  buf->st_ino = reinterpret_cast<uintptr_t>(node.get());

  // Set dummy timestamps
  buf->st_atime = 1000000;
  buf->st_mtime = 1000000;
  buf->st_ctime = 1000000;
  
  if (node->is_directory()) {
      buf->st_mode = S_IFDIR | 0755;
      buf->st_nlink = 2;
      if (buf->st_size == 0) buf->st_size = 4096;
  } else if (node->is_symlink()) {
      buf->st_mode = S_IFLNK | 0777;
      buf->st_nlink = 1;
  } else {
      buf->st_mode = S_IFREG | 0644;
      buf->st_nlink = 1;
  }

  fk::algorithms::klog("SYSCALL", "sys_fstat: fd=%lu mode=%o size=%zu ino=%p", fd, buf->st_mode, (size_t)buf->st_size, (void*)buf->st_ino);

  return 0;
}
}
