#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/MqueueFs/mqueue_dir_node.h>
#include <Kernel/Fs/Virtual/MqueueFs/mqueue_node.h>
#include <Kernel/Fs/Vfs/Core/definitions.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

// sys_mq_open(...) → 0 or -errno
extern "C" uint64_t sys_mq_open(uint64_t name_ptr, uint64_t oflag, [[maybe_unused]] uint64_t mode,
                      [[maybe_unused]] uint64_t attr_ptr, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) {
  const char* name = reinterpret_cast<const char*>(name_ptr);
  if (!name || name[0] != '/') return (uint64_t)-22;

  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  char path[256];
  fk::memory::copy_string(path, "/dev/mqueue");
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

  auto mq_res = MqueueNode::create(10, 8192);
  if (mq_res.is_error()) return (uint64_t)-12;

  auto dentry_res = Dentry::create("mqueue", nullptr);
  if (dentry_res.is_error()) return (uint64_t)-12;
  auto dentry = dentry_res.value();
  dentry->push_node(mq_res.value());

  auto desc = fk::make_ref<FileDescription>(dentry, O_RDWR).value();
  int fd = task->add_file_descriptor(desc);
  return (uint64_t)fd;
}
