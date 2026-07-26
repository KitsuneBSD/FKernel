#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/MqueueFs/mqueue_dir_node.h>
#include <Kernel/Fs/Virtual/MqueueFs/mqueue_node.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

static fk::RefPtr<MqueueNode> resolve_mq(int fd) {
  auto* task = SchedulerManager::the().current();
  if (!task) return nullptr;
  auto desc = task->get_file_descriptor(fd);
  if (!desc) return nullptr;
  auto node = desc->node();
  if (!node || !node->is_mqueue()) return nullptr;
  return fk::RefPtr<MqueueNode>(static_cast<MqueueNode*>(node.get()));
}

extern "C" {

uint64_t sys_mq_open(uint64_t name_ptr, uint64_t oflag, [[maybe_unused]] uint64_t mode,
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

uint64_t sys_mq_send(uint64_t fd, uint64_t msg_ptr, uint64_t msg_len,
                      uint64_t msg_prio, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) {
  auto mq = resolve_mq((int)fd);
  if (!mq) return (uint64_t)-9;

  auto* buf = reinterpret_cast<const void*>(msg_ptr);
  if (!buf) return (uint64_t)-14;
  if (msg_len == 0) return 0;

  return (uint64_t)mq->send(buf, (size_t)msg_len, (uint32_t)msg_prio);
}

uint64_t sys_mq_receive(uint64_t fd, uint64_t msg_ptr, uint64_t msg_len,
                         uint64_t prio_ptr, uint64_t, uint64_t,
                         [[maybe_unused]] PtRegs* regs) {
  auto mq = resolve_mq((int)fd);
  if (!mq) return (uint64_t)-9;

  auto* buf = reinterpret_cast<void*>(msg_ptr);
  if (!buf) return (uint64_t)-14;

  uint32_t prio = 0;
  ssize_t received = mq->receive(buf, (size_t)msg_len, &prio, false);
  if (received < 0) return (uint64_t)received;

  if (prio_ptr) {
    auto res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(prio_ptr), &prio, sizeof(uint32_t));
    if (res.is_error()) return (uint64_t)-14;
  }
  return (uint64_t)received;
}

uint64_t sys_mq_unlink(uint64_t name_ptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                        [[maybe_unused]] PtRegs* regs) {
  const char* name = reinterpret_cast<const char*>(name_ptr);
  if (!name || name[0] != '/') return (uint64_t)-22;

  char path[256];
  fk::memory::copy_string(path, "/dev/mqueue");
  fk::memory::concatenate(path, name);
  path[255] = 0;

  auto dentry_res = VirtualFileSystem::the().resolve_path(path);
  if (dentry_res.is_error()) return (uint64_t)-2;

  return 0;
}

}
