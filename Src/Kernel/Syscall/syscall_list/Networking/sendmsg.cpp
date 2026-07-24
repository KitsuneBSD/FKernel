#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Net/socket.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>

struct KernelIovec {
    void*  iov_base;
    size_t iov_len;
};

struct KernelMsghdr {
    void*       msg_name;
    uint32_t    msg_namelen;
    KernelIovec* msg_iov;
    size_t      msg_iovlen;
    void*       msg_control;
    size_t      msg_controllen;
    int         msg_flags;
};

extern "C" {

uint64_t sys_sendmsg(uint64_t sockfd, uint64_t msghdr_ptr,
                     [[maybe_unused]] uint64_t flags, uint64_t, uint64_t, uint64_t,
                     [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto description = task->get_file_descriptor(static_cast<int>(sockfd));
    if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = description->node();
    if (!node || !node->is_socket()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    KernelMsghdr hdr;
    auto copy_res = fkernel::memory::copy_from_user(
        &hdr, reinterpret_cast<const void*>(msghdr_ptr), sizeof(KernelMsghdr));
    if (copy_res.is_error()) return -static_cast<int>(copy_res.error());

    static constexpr size_t MAX_IOV = 16;
    if (hdr.msg_iovlen > MAX_IOV) return -static_cast<int>(fk::core::Error::InvalidParameter);

    KernelIovec iov[MAX_IOV];
    auto copy_iov = fkernel::memory::copy_from_user(
        iov, hdr.msg_iov, hdr.msg_iovlen * sizeof(KernelIovec));
    if (copy_iov.is_error()) return -static_cast<int>(copy_iov.error());

    auto* socket = static_cast<fkernel::Socket*>(node.get());
    const char* addr = reinterpret_cast<const char*>(hdr.msg_name);
    uint32_t addrlen = hdr.msg_namelen;

    size_t total = 0;
    uint8_t kbuf[4096];
    for (size_t i = 0; i < hdr.msg_iovlen; ++i) {
        size_t chunk = iov[i].iov_len < sizeof(kbuf) ? iov[i].iov_len : sizeof(kbuf);
        auto copy_data = fkernel::memory::copy_from_user(kbuf, iov[i].iov_base, chunk);
        if (copy_data.is_error()) return -static_cast<int>(copy_data.error());
        auto res = socket->sendto(chunk, kbuf, addr, addrlen);
        if (res.is_error()) return -static_cast<int>(res.error());
        total += res.value();
    }
    return total;
}

}
