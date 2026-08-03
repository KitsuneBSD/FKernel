#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Net/Sockets/socket.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>

extern "C" {

uint64_t sys_getsockopt(uint64_t sockfd, uint64_t level, uint64_t optname,
                         uint64_t optval, uint64_t optlen_ptr,
                         uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto description = task->get_file_descriptor(static_cast<int>(sockfd));
    if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = description->node();
    if (!node || !node->is_socket()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    uint32_t optlen_val = 0;
    auto copy_len = fkernel::memory::copy_from_user(
        &optlen_val, reinterpret_cast<const void*>(optlen_ptr), sizeof(uint32_t));
    if (copy_len.is_error()) return -static_cast<int>(copy_len.error());

    if (optlen_val > 128) optlen_val = 128;

    uint8_t kbuf[128] = {};
    auto* socket = static_cast<fkernel::Socket*>(node.get());
    auto res = socket->getsockopt(
        static_cast<int>(level), static_cast<int>(optname), kbuf, &optlen_val);
    if (res.is_error()) return -static_cast<int>(res.error());

    auto copy_val = fkernel::memory::copy_to_user(
        reinterpret_cast<void*>(optval), kbuf, optlen_val);
    if (copy_val.is_error()) return -static_cast<int>(copy_val.error());

    auto copy_back = fkernel::memory::copy_to_user(
        reinterpret_cast<void*>(optlen_ptr), &optlen_val, sizeof(uint32_t));
    if (copy_back.is_error()) return -static_cast<int>(copy_back.error());

    return 0;
}

}
