#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Net/socket.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>

extern "C" {

uint64_t sys_sendto(uint64_t sockfd, uint64_t buf, uint64_t len,
                    [[maybe_unused]] uint64_t flags, uint64_t addr, uint64_t addrlen,
                    [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto description = task->get_file_descriptor(static_cast<int>(sockfd));
    if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = description->node();
    if (!node || !node->is_socket()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    if (!fkernel::memory::is_user_address(buf, len))
        return -static_cast<int>(fk::core::Error::InvalidParameter);

    uint8_t kbuf[4096];
    size_t remaining = len;
    size_t total_sent = 0;
    auto* socket = static_cast<fkernel::Socket*>(node.get());
    const char* user_addr = addr ? reinterpret_cast<const char*>(addr) : nullptr;

    while (remaining > 0) {
        size_t chunk = remaining < sizeof(kbuf) ? remaining : sizeof(kbuf);
        auto copy_res = fkernel::memory::copy_from_user(kbuf,
            reinterpret_cast<const void*>(buf + total_sent), chunk);
        if (copy_res.is_error()) return -static_cast<int>(copy_res.error());

        auto res = socket->sendto(chunk, kbuf, user_addr, (uint32_t)addrlen);
        if (res.is_error()) return -static_cast<int>(res.error());

        total_sent  += res.value();
        remaining   -= chunk;
    }

    return total_sent;
}

}
