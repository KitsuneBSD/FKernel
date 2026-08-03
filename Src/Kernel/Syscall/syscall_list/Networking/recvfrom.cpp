#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Net/Sockets/socket.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/error.h>

extern "C" {

uint64_t sys_recvfrom(uint64_t sockfd, uint64_t buf, uint64_t len,
                      [[maybe_unused]] uint64_t flags, uint64_t addr, uint64_t addrlen_ptr,
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
    size_t to_recv = len < sizeof(kbuf) ? len : sizeof(kbuf);

    char   addr_buf[128] = {};
    uint32_t addrlen_val = 0;
    if (addrlen_ptr) {
        auto copy_res = fkernel::memory::copy_from_user(
            &addrlen_val, reinterpret_cast<const void*>(addrlen_ptr), sizeof(uint32_t));
        if (copy_res.is_error()) return -static_cast<int>(copy_res.error());
    }

    auto* socket = static_cast<fkernel::Socket*>(node.get());
    char* out_addr = addr ? addr_buf : nullptr;
    uint32_t* out_addrlen = addrlen_ptr ? &addrlen_val : nullptr;

    auto res = socket->recvfrom(to_recv, kbuf, out_addr, out_addrlen);
    if (res.is_error()) return -static_cast<int>(res.error());

    size_t received = res.value();
    auto copy_out = fkernel::memory::copy_to_user(
        reinterpret_cast<void*>(buf), kbuf, received);
    if (copy_out.is_error()) return -static_cast<int>(copy_out.error());

    if (addr && addrlen_ptr && addrlen_val > 0) {
        auto copy_addr = fkernel::memory::copy_to_user(
            reinterpret_cast<void*>(addr), addr_buf, addrlen_val);
        if (copy_addr.is_error()) return -static_cast<int>(copy_addr.error());
        auto copy_len = fkernel::memory::copy_to_user(
            reinterpret_cast<void*>(addrlen_ptr), &addrlen_val, sizeof(uint32_t));
        if (copy_len.is_error()) return -static_cast<int>(copy_len.error());
    }

    return received;
}

}
