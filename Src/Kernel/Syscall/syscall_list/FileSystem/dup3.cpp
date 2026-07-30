#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" {

uint64_t sys_dup3(uint64_t oldfd_u64, uint64_t newfd_u64, uint64_t flags,
                  uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    int oldfd = (int)oldfd_u64;
    int newfd = (int)newfd_u64;

    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    if (oldfd == newfd) return (uint64_t)-22; // EINVAL — dup3 requires oldfd != newfd
    if (newfd < 0 || newfd >= static_cast<int>(MAX_OPEN_FILES)) return (uint64_t)-22;

    auto desc = task->get_file_descriptor(oldfd);
    if (!desc) return (uint64_t)-9; // EBADF

    bool cloexec = (flags & 02000000) != 0; // O_CLOEXEC
    desc->set_cloexec(cloexec);

    if (task->install_at(newfd, desc) < 0)
        return (uint64_t)-22; // EINVAL

    return (uint64_t)newfd;
}

}
