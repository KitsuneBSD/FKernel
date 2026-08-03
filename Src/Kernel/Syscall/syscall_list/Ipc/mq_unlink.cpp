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

// sys_mq_unlink(...) → 0 or -errno
extern "C" uint64_t sys_mq_unlink(uint64_t name_ptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
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
