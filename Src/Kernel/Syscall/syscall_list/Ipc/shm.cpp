#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/ShmFs/shm_dir_node.h>
#include <Kernel/Fs/Virtual/ShmFs/shm_node.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

extern "C" {

uint64_t sys_shm_open(uint64_t name_ptr, uint64_t oflag, [[maybe_unused]] uint64_t mode,
                       uint64_t, uint64_t, uint64_t,
                       [[maybe_unused]] PtRegs* regs) {
  const char* name = reinterpret_cast<const char*>(name_ptr);
  if (!name || name[0] != '/') return (uint64_t)-22;

  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  char path[256];
  fk::memory::copy_string(path, "/dev/shm");
  fk::memory::concatenate(path, name);
  path[255] = 0;

  bool create = ((int)oflag & O_CREAT) != 0;

  auto existing_res = VirtualFileSystem::the().resolve_path(path);
  if (!existing_res.is_error()) {
    auto dentry = existing_res.value();
    auto desc = fk::make_ref<FileDescription>(dentry, O_RDWR).value();
    int fd = task->add_file_descriptor(desc);
    return (uint64_t)fd;
  }

  if (!create) return (uint64_t)-2;

  auto shm_res = ShmNode::create();
  if (shm_res.is_error()) return (uint64_t)-12;

  auto dentry_res = Dentry::create("shm", nullptr);
  if (dentry_res.is_error()) return (uint64_t)-12;
  auto dentry = dentry_res.value();
  dentry->push_node(shm_res.value());

  auto desc = fk::make_ref<FileDescription>(dentry, O_RDWR).value();
  int fd = task->add_file_descriptor(desc);
  return (uint64_t)fd;
}

uint64_t sys_shm_unlink(uint64_t name_ptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                         [[maybe_unused]] PtRegs* regs) {
  const char* name = reinterpret_cast<const char*>(name_ptr);
  if (!name || name[0] != '/') return (uint64_t)-22;

  char path[256];
  fk::memory::copy_string(path, "/dev/shm");
  fk::memory::concatenate(path, name);
  path[255] = 0;

  auto dentry_res = VirtualFileSystem::the().resolve_path(path);
  if (dentry_res.is_error()) return (uint64_t)-2;

  return 0;
}

}
