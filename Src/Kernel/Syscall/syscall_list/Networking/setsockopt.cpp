#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Net/Sockets/socket.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>

extern "C" {

uint64_t sys_setsockopt(uint64_t sockfd, uint64_t level, uint64_t optname,
                         uint64_t optval, uint64_t optlen,
                         uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto description = task->get_file_descriptor(static_cast<int>(sockfd));
    if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = description->node();
    if (!node || !node->is_socket()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    if (optlen > 128) return -static_cast<int>(fk::core::Error::InvalidParameter);

    uint8_t kbuf[128] = {};
    auto copy_res = fkernel::memory::copy_from_user(
        kbuf, reinterpret_cast<const void*>(optval), optlen);
    if (copy_res.is_error()) return -static_cast<int>(copy_res.error());

    auto* socket = static_cast<fkernel::Socket*>(node.get());
    auto res = socket->setsockopt(
        static_cast<int>(level), static_cast<int>(optname), kbuf, (uint32_t)optlen);
    if (res.is_error()) return -static_cast<int>(res.error());

    return 0;
}

}
