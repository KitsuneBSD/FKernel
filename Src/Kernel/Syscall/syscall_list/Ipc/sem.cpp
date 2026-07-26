#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/SemFs/sem_dir_node.h>
#include <Kernel/Fs/Virtual/SemFs/sem_node.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

static fk::RefPtr<SemNode> resolve_sem(int fd) {
  auto* task = SchedulerManager::the().current();
  if (!task) return nullptr;
  auto desc = task->get_file_descriptor(fd);
  if (!desc) return nullptr;
  auto node = desc->node();
  if (!node || !node->is_semaphore()) return nullptr;
  return fk::RefPtr<SemNode>(static_cast<SemNode*>(node.get()));
}

extern "C" {

uint64_t sys_sem_open(uint64_t name_ptr, uint64_t oflag, [[maybe_unused]] uint64_t mode, uint64_t value,
                       uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  const char* name = reinterpret_cast<const char*>(name_ptr);
  if (!name || name[0] != '/') return (uint64_t)-22;

  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  char path[256];
  fk::memory::copy_string(path, "/dev/sem");
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

  auto sem_res = SemNode::create((uint32_t)value, ~0u);
  if (sem_res.is_error()) return (uint64_t)-12;

  auto dentry_res = Dentry::create("sem", nullptr);
  if (dentry_res.is_error()) return (uint64_t)-12;
  auto dentry = dentry_res.value();
  dentry->push_node(sem_res.value());

  auto desc = fk::make_ref<FileDescription>(dentry, O_RDWR).value();
  int fd = task->add_file_descriptor(desc);
  return (uint64_t)fd;
}

uint64_t sys_sem_wait(uint64_t fd, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                       [[maybe_unused]] PtRegs* regs) {
  auto sem = resolve_sem((int)fd);
  if (!sem) return (uint64_t)-9;
  return (uint64_t)sem->wait();
}

uint64_t sys_sem_post(uint64_t fd, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                       [[maybe_unused]] PtRegs* regs) {
  auto sem = resolve_sem((int)fd);
  if (!sem) return (uint64_t)-9;
  return (uint64_t)sem->post();
}

uint64_t sys_sem_getvalue(uint64_t fd, uint64_t val_ptr, uint64_t, uint64_t, uint64_t, uint64_t,
                           [[maybe_unused]] PtRegs* regs) {
  auto sem = resolve_sem((int)fd);
  if (!sem) return (uint64_t)-9;
  int val = sem->get_value();
  if (val_ptr) {
    auto res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(val_ptr), &val, sizeof(int));
    if (res.is_error()) return (uint64_t)-14;
  }
  return 0;
}

uint64_t sys_sem_unlink(uint64_t name_ptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                         [[maybe_unused]] PtRegs* regs) {
  const char* name = reinterpret_cast<const char*>(name_ptr);
  if (!name || name[0] != '/') return (uint64_t)-22;

  char path[256];
  fk::memory::copy_string(path, "/dev/sem");
  fk::memory::concatenate(path, name);
  path[255] = 0;

  auto dentry_res = VirtualFileSystem::the().resolve_path(path);
  if (dentry_res.is_error()) return (uint64_t)-2;

  return 0;
}

}
