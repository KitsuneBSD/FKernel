#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Net/socket.h>
#include <Kernel/Net/unix_socket.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Core/error.h>
#include <LibFK/Utilities/memory.h>

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

// Kernel-side cmsghdr: cmsg_len uses size_t (8 bytes) on x86_64 to match glibc layout.
struct KernelCmsghdr {
    size_t cmsg_len;
    int    cmsg_level;
    int    cmsg_type;
};

static constexpr int SOL_SOCKET  = 1;
static constexpr int SCM_RIGHTS  = 1;
static constexpr size_t CMSG_ALIGN_VAL = 8;
static constexpr size_t CMSG_HDR_SIZE  = sizeof(KernelCmsghdr); // 16 bytes on x86_64

static size_t cmsg_align(size_t n) {
    return (n + CMSG_ALIGN_VAL - 1) & ~(CMSG_ALIGN_VAL - 1);
}

// Process one SCM_RIGHTS cmsg: build a vector of FileDescriptions and hand to unix socket.
static void handle_scm_rights(Task* task, fkernel::UnixSocket* usock,
                               const uint8_t* data, size_t data_len) {
    size_t fd_count = data_len / sizeof(int);
    fk::containers::Vector<fk::RefPtr<FileDescription>> fds;
    for (size_t i = 0; i < fd_count; ++i) {
        int fd = 0;
        fk::memory::copy(&fd, data + i * sizeof(int), sizeof(int));
        auto desc = task->get_file_descriptor(fd);
        if (desc) fds.push_back(fk::types::move(desc));
    }
    usock->send_fds(fds);
}

// Walk the control message buffer and dispatch SCM_RIGHTS entries to the unix socket.
static void process_control_messages(Task* task, fkernel::Socket* socket,
                                     const uint8_t* cmsg_buf, size_t cmsg_len) {
    if (socket->domain() != fkernel::SocketDomain::Unix) return;
    auto* usock = static_cast<fkernel::UnixSocket*>(socket);
    size_t offset = 0;
    while (offset + CMSG_HDR_SIZE <= cmsg_len) {
        KernelCmsghdr hdr;
        fk::memory::copy(&hdr, cmsg_buf + offset, CMSG_HDR_SIZE);
        if (hdr.cmsg_len < CMSG_HDR_SIZE) break;
        size_t data_len = hdr.cmsg_len - CMSG_HDR_SIZE;
        if (offset + CMSG_HDR_SIZE + data_len > cmsg_len) break;
        if (hdr.cmsg_level == SOL_SOCKET && hdr.cmsg_type == SCM_RIGHTS)
            handle_scm_rights(task, usock, cmsg_buf + offset + CMSG_HDR_SIZE, data_len);
        offset += cmsg_align(hdr.cmsg_len);
    }
}

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

    static constexpr size_t MAX_IOV     = 16;
    static constexpr size_t MAX_CMSG    = 4096;
    if (hdr.msg_iovlen > MAX_IOV) return -static_cast<int>(fk::core::Error::InvalidParameter);

    KernelIovec iov[MAX_IOV];
    auto copy_iov = fkernel::memory::copy_from_user(
        iov, hdr.msg_iov, hdr.msg_iovlen * sizeof(KernelIovec));
    if (copy_iov.is_error()) return -static_cast<int>(copy_iov.error());

    auto* socket = static_cast<fkernel::Socket*>(node.get());

    // Process ancillary (control) messages before sending data.
    if (hdr.msg_control && hdr.msg_controllen > 0 && hdr.msg_controllen <= MAX_CMSG) {
        uint8_t cmsg_buf[MAX_CMSG];
        auto copy_ctrl = fkernel::memory::copy_from_user(cmsg_buf, hdr.msg_control, hdr.msg_controllen);
        if (copy_ctrl.is_ok())
            process_control_messages(task, socket, cmsg_buf, hdr.msg_controllen);
    }

    const char* addr   = reinterpret_cast<const char*>(hdr.msg_name);
    uint32_t addrlen   = hdr.msg_namelen;
    size_t total       = 0;
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
