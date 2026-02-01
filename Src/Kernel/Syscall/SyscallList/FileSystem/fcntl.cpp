#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

// fcntl commands (Linux values)
#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_DUPFD_CLOEXEC 1030

uint64_t sys_fcntl([[maybe_unused]] uint64_t fd, uint64_t cmd, [[maybe_unused]] uint64_t arg,
                   [[maybe_unused]] uint64_t arg4, [[maybe_unused]] uint64_t arg5,
                   [[maybe_unused]] uint64_t arg6, [[maybe_unused]] PtRegs* regs) {

  auto* task = SchedulerManager::the().current();
  if (!task)
    return fkernel::return_error(fk::core::Error::PermissionDenied);

  // Simple stub for now
  switch (cmd) {
  case F_DUPFD:
    return static_cast<uint64_t>(
        task->dup_file_descriptor(static_cast<int>(fd), false, static_cast<int>(arg)));
  case F_DUPFD_CLOEXEC:
    return static_cast<uint64_t>(
        task->dup_file_descriptor(static_cast<int>(fd), true, static_cast<int>(arg)));
  case F_GETFD:
    return 0; // No FD_CLOEXEC for now
  case F_SETFD:
    return 0; // Success
  case F_GETFL:
    return 0; // O_RDONLY
  case F_SETFL:
    return 0; // Success
  default:
    fk::algorithms::kwarn("SYSCALL", "fcntl: Unsupported command %lu", cmd);
    return fkernel::return_error(fk::core::Error::NotImplemented);
  }
}
}
