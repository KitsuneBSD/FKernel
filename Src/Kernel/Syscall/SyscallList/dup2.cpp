#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/Error.h>

extern "C" {
uint64_t sys_dup2(uint64_t oldfd_u64, uint64_t newfd_u64, uint64_t, uint64_t,
                  uint64_t, uint64_t, PtRegs* regs) {
    int oldfd = (int)oldfd_u64;
    int newfd = (int)newfd_u64;

    auto* current = SchedulerManager::the().current();
    if (!current) return -1;

    auto desc = current->get_file_descriptor(oldfd);
    if (!desc) return -static_cast<int>(fk::core::Error::InvalidHandle);

    if (oldfd == newfd) return newfd;

    // Em nosso sistema, o add_file_descriptor em um índice específico ainda não existe.
    // Vamos simplificar: se o slot já existe, sobrescrevemos.
    if (newfd < 0 || newfd >= 32) return -static_cast<int>(fk::core::Error::InvalidParameter);

    current->file_descriptors[newfd] = desc;
    return newfd;
}
}
