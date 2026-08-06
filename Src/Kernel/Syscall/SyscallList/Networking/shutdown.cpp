#include <LibFK/Core/error.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Net/Sockets/socket.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" {

uint64_t sys_shutdown(uint64_t sockfd, uint64_t how,
                      uint64_t, uint64_t, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto description = task->get_file_descriptor(static_cast<int>(sockfd));
    if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = description->node();
    if (!node || !node->is_socket()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    auto* socket = static_cast<fkernel::Socket*>(node.get());
    auto res = socket->shutdown(static_cast<int>(how));
    if (res.is_error()) return -static_cast<int>(res.error());

    return 0;
}

}
