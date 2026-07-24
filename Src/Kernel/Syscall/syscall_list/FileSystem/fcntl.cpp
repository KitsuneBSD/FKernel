#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

#define F_DUPFD          0
#define F_GETFD          1
#define F_SETFD          2
#define F_GETFL          3
#define F_SETFL          4
#define F_GETLK          5
#define F_SETLK          6
#define F_SETLKW         7
#define F_DUPFD_CLOEXEC  1030
#define FD_CLOEXEC       1

#define F_RDLCK   0
#define F_WRLCK   1
#define F_UNLCK   2

// Flags that userspace may change via F_SETFL (exclude access mode bits 0-1)
#define O_SETFL_MASK   (04010000) // O_APPEND|O_NONBLOCK|O_ASYNC|O_DIRECT|O_NOATIME

struct flock {
  short l_type;
  short l_whence;
  long  l_start;
  long  l_len;
  int   l_pid;
};

uint64_t sys_fcntl(uint64_t fd, uint64_t cmd, uint64_t arg,
                   uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);

    switch (cmd) {
    case F_DUPFD:
        return (uint64_t)task->dup_file_descriptor((int)fd, false, (int)arg);
    case F_DUPFD_CLOEXEC:
        return (uint64_t)task->dup_file_descriptor((int)fd, true, (int)arg);
    case F_GETFD: {
        auto desc = task->get_file_descriptor((int)fd);
        if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
        return desc->is_cloexec() ? FD_CLOEXEC : 0;
    }
    case F_SETFD: {
        auto desc = task->get_file_descriptor((int)fd);
        if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
        desc->set_cloexec(arg & FD_CLOEXEC);
        return 0;
    }
    case F_GETFL: {
        auto desc = task->get_file_descriptor((int)fd);
        if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
        return (uint64_t)desc->open_flags();
    }
    case F_SETFL: {
        auto desc = task->get_file_descriptor((int)fd);
        if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
        int new_flags = (desc->open_flags() & ~O_SETFL_MASK) | ((int)arg & O_SETFL_MASK);
        desc->set_open_flags(new_flags);
        return 0;
    }
    case F_GETLK: {
        auto* fl = reinterpret_cast<struct flock*>(arg);
        if (!fl) return fkernel::return_error(fk::core::Error::InvalidParameter);
        fl->l_type = F_UNLCK;
        fl->l_pid = -1;
        return 0;
    }
    case F_SETLK:
    case F_SETLKW:
        return 0;
    default:
        fk::algorithms::kwarn("SYSCALL", "fcntl: unsupported cmd %lu", cmd);
        return fkernel::return_error(fk::core::Error::NotImplemented);
    }
}
}
