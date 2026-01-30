#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/Error.h>

extern "C" {
uint64_t sys_dup2(uint64_t oldfd_u64, uint64_t newfd_u64, uint64_t, uint64_t,
                  uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    int oldfd = (int)oldfd_u64;
    int newfd = (int)newfd_u64;

    auto* current = SchedulerManager::the().current();
    if (!current) return -1;

    auto desc = current->get_file_descriptor(oldfd);
    if (!desc) return -static_cast<int>(fk::core::Error::InvalidHandle);

    if (oldfd == newfd) return newfd;

    if (newfd < 0 || newfd >= 32) return -static_cast<int>(fk::core::Error::InvalidParameter);

    current->resources.files.descriptors[newfd] = desc;
    return newfd;
}
}