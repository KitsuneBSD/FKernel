#include <Kernel/Fs/Virtual/PipeFs/pipe_node.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Memory/ref_ptr.h>

using namespace fkernel;

extern "C" {

uint64_t sys_pipe2(uint64_t pipefd_ptr, uint64_t flags, uint64_t, uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    auto pipe_res = PipeNode::create();
    if (pipe_res.is_error()) return (uint64_t)-(int)pipe_res.error();

    auto dentry_res = Dentry::create("pipe", nullptr);
    if (dentry_res.is_error()) return (uint64_t)-1;

    auto dentry = dentry_res.value();
    auto pipe = pipe_res.value();
    dentry->push_node(pipe);

    bool cloexec  = (flags & 02000000) != 0;
    bool nonblock = (flags & O_NONBLOCK) != 0;

    int rflags = O_RDONLY | (nonblock ? O_NONBLOCK : 0);
    int wflags = O_WRONLY | (nonblock ? O_NONBLOCK : 0);
    auto read_desc  = fk::make_ref<FileDescription>(dentry, rflags).value();
    auto write_desc = fk::make_ref<FileDescription>(dentry, wflags).value();
    if (cloexec) { read_desc->set_cloexec(true); write_desc->set_cloexec(true); }
    pipe->add_reader();
    pipe->set_read_nonblock(nonblock);
    pipe->set_write_nonblock(nonblock);

    int fd_r = task->add_file_descriptor(read_desc);
    if (fd_r < 0) return (uint64_t)fd_r;
    int fd_w = task->add_file_descriptor(write_desc);
    if (fd_w < 0) { task->close_file_descriptor(fd_r); return (uint64_t)fd_w; }

    int* user_fds = reinterpret_cast<int*>(pipefd_ptr);
    if (!user_fds) { task->close_file_descriptor(fd_r); task->close_file_descriptor(fd_w); return (uint64_t)-14; }
    user_fds[0] = fd_r;
    user_fds[1] = fd_w;
    return 0;
}

}
