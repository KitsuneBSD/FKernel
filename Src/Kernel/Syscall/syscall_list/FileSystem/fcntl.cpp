#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>

struct flock {
  short l_type;
  short l_whence;
  long  l_start;
  long  l_len;
  int   l_pid;
};

static constexpr short F_UNLCK = 2;

extern "C" {
uint64_t sys_fcntl(uint64_t fd_u64, uint64_t cmd, uint64_t arg_u64,
                   uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int fd = static_cast<int>(fd_u64);
  uint64_t arg = arg_u64;

  auto* task = SchedulerManager::the().current();
  if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);

  static constexpr uint64_t F_DUPFD = 0;
  static constexpr uint64_t F_DUPFD_CLOEXEC = 1030;
  if (cmd == F_DUPFD)
    return static_cast<uint64_t>(task->dup_file_descriptor(fd, false, static_cast<int>(arg)));
  if (cmd == F_DUPFD_CLOEXEC)
    return static_cast<uint64_t>(task->dup_file_descriptor(fd, true, static_cast<int>(arg)));

  static constexpr uint64_t F_GETFD = 1;
  static constexpr uint64_t F_SETFD = 2;
  if (cmd == F_GETFD) {
    auto desc = task->get_file_descriptor(fd);
    if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
    return desc->is_cloexec() ? 1 : 0;
  }
  if (cmd == F_SETFD) {
    auto desc = task->get_file_descriptor(fd);
    if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
    desc->set_cloexec(arg & 1);
    return 0;
  }

  static constexpr uint64_t F_GETFL = 3;
  static constexpr uint64_t F_SETFL = 4;
  if (cmd == F_GETFL) {
    auto desc = task->get_file_descriptor(fd);
    if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
    return desc->open_flags();
  }
  if (cmd == F_SETFL) {
    auto desc = task->get_file_descriptor(fd);
    if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
    desc->set_open_flags(static_cast<int>(arg));
    return 0;
  }

  static constexpr uint64_t F_GETLK = 5;
  if (cmd == F_GETLK) {
    flock kfl;
    if (fkernel::memory::copy_from_user(&kfl, reinterpret_cast<const void*>(arg), sizeof(kfl)).is_error())
      return -14;
    if (kfl.l_whence != 0)
      return fkernel::return_error(fk::core::Error::InvalidParameter);

    auto desc = task->get_file_descriptor(fd);
    if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
    auto node = desc->node();
    if (!node) return fkernel::return_error(fk::core::Error::InvalidHandle);

    int64_t end = (kfl.l_len == 0) ? INT64_MAX : kfl.l_start + kfl.l_len - 1;
    fk::ProcessId conflict_pid;
    short conflict_type;
    if (node->test_lock_conflict(kfl.l_type, kfl.l_start, end, &conflict_pid, &conflict_type)) {
      kfl.l_type = conflict_type;
      kfl.l_pid = (int)conflict_pid.value();
    } else {
      kfl.l_type = F_UNLCK;
      kfl.l_pid = -1;
    }
    if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &kfl, sizeof(kfl)).is_error())
      return -14;
    return 0;
  }

  static constexpr uint64_t F_SETLK = 6;
  static constexpr uint64_t F_SETLKW = 7;
  if (cmd == F_SETLK || cmd == F_SETLKW) {
    flock kfl;
    if (fkernel::memory::copy_from_user(&kfl, reinterpret_cast<const void*>(arg), sizeof(kfl)).is_error())
      return -14;
    if (kfl.l_whence != 0)
      return fkernel::return_error(fk::core::Error::InvalidParameter);
    if (kfl.l_len < 0)
      return fkernel::return_error(fk::core::Error::InvalidParameter);

    auto desc = task->get_file_descriptor(fd);
    if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
    auto node = desc->node();
    if (!node) return fkernel::return_error(fk::core::Error::InvalidHandle);

    auto pid = task->control.identity.id;
    int64_t end = (kfl.l_len == 0) ? INT64_MAX : kfl.l_start + kfl.l_len - 1;

    if (kfl.l_type == F_UNLCK) {
      node->release_lock(pid, kfl.l_start, end);
      return 0;
    }

    if (cmd == F_SETLK) {
      if (!node->try_acquire_lock(pid, kfl.l_type, kfl.l_start, end, nullptr, nullptr))
        return fkernel::return_error(fk::core::Error::WouldBlock);
      return 0;
    }

    while (true) {
      if (task->has_pending_signals())
        return (uint64_t)(-4);
      if (node->try_acquire_lock(pid, kfl.l_type, kfl.l_start, end, nullptr, nullptr))
        return 0;
      SchedulerManager::the().sleep_current(fk::TickCount(10));
      SchedulerManager::the().schedule();
    }
  }

  fk::algorithms::kwarn("SYSCALL", "fcntl: unsupported cmd %lu", cmd);
  return fkernel::return_error(fk::core::Error::InvalidParameter);
}
}
