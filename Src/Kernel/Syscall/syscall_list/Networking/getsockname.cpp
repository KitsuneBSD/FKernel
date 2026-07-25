#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Net/socket.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>

extern "C" {

uint64_t sys_getsockname(uint64_t sockfd, uint64_t addr, uint64_t addrlen_ptr,
                          uint64_t, uint64_t, uint64_t,
                          [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto description = task->get_file_descriptor(static_cast<int>(sockfd));
    if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = description->node();
    if (!node || !node->is_socket()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    uint32_t addrlen_val = 0;
    auto copy_len = fkernel::memory::copy_from_user(
        &addrlen_val, reinterpret_cast<const void*>(addrlen_ptr), sizeof(uint32_t));
    if (copy_len.is_error()) return -static_cast<int>(copy_len.error());

    char addr_buf[128] = {};
    auto* socket = static_cast<fkernel::Socket*>(node.get());
    auto res = socket->getsockname(addr_buf, &addrlen_val);
    if (res.is_error()) return -static_cast<int>(res.error());

    auto copy_addr = fkernel::memory::copy_to_user(
        reinterpret_cast<void*>(addr), addr_buf, addrlen_val);
    if (copy_addr.is_error()) return -static_cast<int>(copy_addr.error());

    auto copy_back = fkernel::memory::copy_to_user(
        reinterpret_cast<void*>(addrlen_ptr), &addrlen_val, sizeof(uint32_t));
    if (copy_back.is_error()) return -static_cast<int>(copy_back.error());

    return 0;
}

}
