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

// sys_mq_send(...) → 0 or -errno
extern "C" uint64_t sys_mq_send(uint64_t fd, uint64_t msg_ptr, uint64_t msg_len,
                      uint64_t msg_prio, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) {
  auto mq = resolve_mq((int)fd);
  if (!mq) return (uint64_t)-9;

  auto* buf = reinterpret_cast<const void*>(msg_ptr);
  if (!buf) return (uint64_t)-14;
  if (msg_len == 0) return 0;

  return (uint64_t)mq->send(buf, (size_t)msg_len, (uint32_t)msg_prio);
}
