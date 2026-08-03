#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Net/Sockets/socket.h>
#include <Kernel/Net/Sockets/unix_socket.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Core/error.h>
#include <LibFK/Utilities/memory.h>

struct KernelIovec {
    void*  iov_base;
    size_t iov_len;
};

struct KernelMsghdr {
    void*        msg_name;
    uint32_t     msg_namelen;
    KernelIovec* msg_iov;
    size_t       msg_iovlen;
    void*        msg_control;
    size_t       msg_controllen;
    int          msg_flags;
};

// Kernel-side cmsghdr: cmsg_len uses size_t (8 bytes) on x86_64 to match glibc layout.
struct KernelCmsghdr {
    size_t cmsg_len;
    int    cmsg_level;
    int    cmsg_type;
};

static constexpr int SOL_SOCKET    = 1;
static constexpr int SCM_RIGHTS    = 1;
static constexpr size_t CMSG_ALIGN_VAL = 8;
static constexpr size_t CMSG_HDR_SIZE  = sizeof(KernelCmsghdr); // 16 bytes

static size_t cmsg_align(size_t n) {
    return (n + CMSG_ALIGN_VAL - 1) & ~(CMSG_ALIGN_VAL - 1);
}

// Install pending fds into task's fd table, then write a SCM_RIGHTS cmsg into buf.
// Returns the number of bytes written into buf (0 if no fds or buf too small).
static size_t write_scm_rights_cmsg(
    Task* task,
    fk::containers::Vector<fk::RefPtr<FileDescription>>& fds,
    uint8_t* buf, size_t buf_size) {
    size_t fd_count = fds.size();
    if (fd_count == 0) return 0;
    size_t data_size  = fd_count * sizeof(int);
    size_t total_size = cmsg_align(CMSG_HDR_SIZE + data_size);
    if (total_size > buf_size) return 0;

    KernelCmsghdr hdr;
    hdr.cmsg_len   = CMSG_HDR_SIZE + data_size;
    hdr.cmsg_level = SOL_SOCKET;
    hdr.cmsg_type  = SCM_RIGHTS;
    fk::memory::copy(buf, &hdr, CMSG_HDR_SIZE);

    uint8_t* data_ptr = buf + CMSG_HDR_SIZE;
    for (size_t i = 0; i < fd_count; ++i) {
        int new_fd = task->add_file_descriptor(fk::types::move(fds[i]));
        fk::memory::copy(data_ptr + i * sizeof(int), &new_fd, sizeof(int));
    }
    return total_size;
}

// If socket is a Unix socket, collect any pending SCM_RIGHTS fds and write cmsg to user.
static void deliver_ancillary(Task* task, fkernel::Socket* socket,
                               void* user_ctrl_ptr, size_t ctrl_len,
                               uint64_t msghdr_ptr) {
    if (!user_ctrl_ptr || ctrl_len == 0) return;
    if (socket->domain() != fkernel::SocketDomain::Unix) return;
    auto* usock = static_cast<fkernel::UnixSocket*>(socket);

    fk::containers::Vector<fk::RefPtr<FileDescription>> fds;
    usock->recv_fds(fds);
    if (fds.is_empty()) return;

    static constexpr size_t CMSG_BUF_MAX = 4096;
    if (ctrl_len > CMSG_BUF_MAX) ctrl_len = CMSG_BUF_MAX;

    uint8_t cmsg_buf[CMSG_BUF_MAX];
    fk::memory::set(cmsg_buf, 0, ctrl_len);

    size_t written = write_scm_rights_cmsg(task, fds, cmsg_buf, ctrl_len);
    if (written == 0) return;

    fkernel::memory::copy_to_user(user_ctrl_ptr, cmsg_buf, written);

    // Update msg_controllen in the user's msghdr to reflect bytes written.
    size_t controllen_offset = offsetof(KernelMsghdr, msg_controllen);
    fkernel::memory::copy_to_user(
        reinterpret_cast<uint8_t*>(msghdr_ptr) + controllen_offset,
        &written, sizeof(size_t));
}

extern "C" {

uint64_t sys_recvmsg(uint64_t sockfd, uint64_t msghdr_ptr,
                     [[maybe_unused]] uint64_t flags, uint64_t, uint64_t, uint64_t,
                     [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto description = task->get_file_descriptor(static_cast<int>(sockfd));
    if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = description->node();
    if (!node || !node->is_socket()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    KernelMsghdr hdr;
    auto copy_hdr = fkernel::memory::copy_from_user(
        &hdr, reinterpret_cast<const void*>(msghdr_ptr), sizeof(KernelMsghdr));
    if (copy_hdr.is_error()) return -static_cast<int>(copy_hdr.error());

    static constexpr size_t MAX_IOV = 16;
    if (hdr.msg_iovlen > MAX_IOV) return -static_cast<int>(fk::core::Error::InvalidParameter);

    KernelIovec iov[MAX_IOV];
    auto copy_iov = fkernel::memory::copy_from_user(
        iov, hdr.msg_iov, hdr.msg_iovlen * sizeof(KernelIovec));
    if (copy_iov.is_error()) return -static_cast<int>(copy_iov.error());

    auto* socket = static_cast<fkernel::Socket*>(node.get());

    char     addr_buf[128] = {};
    uint32_t addrlen_val   = hdr.msg_namelen;
    char*    out_addr      = hdr.msg_name ? addr_buf : nullptr;
    uint32_t* out_addrlen  = hdr.msg_name ? &addrlen_val : nullptr;

    size_t total = 0;
    uint8_t kbuf[4096];
    for (size_t i = 0; i < hdr.msg_iovlen; ++i) {
        size_t chunk = iov[i].iov_len < sizeof(kbuf) ? iov[i].iov_len : sizeof(kbuf);
        auto res = socket->recvfrom(chunk, kbuf, out_addr, out_addrlen);
        if (res.is_error()) return -static_cast<int>(res.error());
        size_t received = res.value();
        if (received == 0) break;
        auto copy_out = fkernel::memory::copy_to_user(iov[i].iov_base, kbuf, received);
        if (copy_out.is_error()) return -static_cast<int>(copy_out.error());
        total += received;
        out_addr    = nullptr;
        out_addrlen = nullptr;
    }

    if (hdr.msg_name && addrlen_val > 0) {
        fkernel::memory::copy_to_user(hdr.msg_name, addr_buf, addrlen_val);
        fkernel::memory::copy_to_user(
            reinterpret_cast<uint8_t*>(msghdr_ptr) + offsetof(KernelMsghdr, msg_namelen),
            &addrlen_val, sizeof(uint32_t));
    }

    // Deliver SCM_RIGHTS ancillary data (pending fds from peer's sendmsg).
    deliver_ancillary(task, socket, hdr.msg_control, hdr.msg_controllen, msghdr_ptr);

    return total;
}

}
