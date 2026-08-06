#include <LibFK/Core/error.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Net/Sockets/socket.h>
#include <Kernel/Net/Sockets/unix_socket.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" {

uint64_t sys_socketpair(uint64_t domain, uint64_t type, [[maybe_unused]] uint64_t protocol,
                         uint64_t sv, uint64_t, uint64_t,
                         [[maybe_unused]] PtRegs* regs) {
    if (domain != 1) // AF_UNIX only
        return -static_cast<int>(fk::core::Error::InvalidParameter);

    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    fk::RefPtr<fkernel::UnixSocket> a, b;
    auto res = fkernel::UnixSocket::create_pair(
        static_cast<fkernel::SocketType>(type), a, b);
    if (res.is_error()) return -static_cast<int>(res.error());

    auto make_fd = [&](fk::RefPtr<fkernel::UnixSocket> sock) -> int {
        auto dentry = fkernel::Dentry::create("socketpair", nullptr).value();
        dentry->push_node(fk::RefPtr<Node>(sock.get()));
        auto desc = fk::make_ref<FileDescription>(dentry, O_RDWR).value();
        return task->add_file_descriptor(desc);
    };

    int fd0 = make_fd(a);
    if (fd0 < 0) return -static_cast<int>(fk::core::Error::OutOfMemory);
    int fd1 = make_fd(b);
    if (fd1 < 0) {
        task->close_file_descriptor(fd0);
        return -static_cast<int>(fk::core::Error::OutOfMemory);
    }

    int fds[2] = { fd0, fd1 };
    auto copy_res = fkernel::memory::copy_to_user(
        reinterpret_cast<void*>(sv), fds, sizeof(fds));
    if (copy_res.is_error()) {
        task->close_file_descriptor(fd0);
        task->close_file_descriptor(fd1);
        return -static_cast<int>(copy_res.error());
    }

    return 0;
}

}
