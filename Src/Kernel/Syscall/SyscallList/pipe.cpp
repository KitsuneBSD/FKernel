#include <Kernel/Fs/PipeFs/pipe_node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Memory/ref_ptr.h>

extern "C" {

uint64_t sys_pipe(uint64_t pipefd_ptr, uint64_t, uint64_t, uint64_t,
                  uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return -1;

    auto pipe_res = fkernel::PipeNode::create();
    if (pipe_res.is_error()) return -static_cast<int>(pipe_res.error());
    auto pipe = pipe_res.value();

    auto read_desc = fk::make_ref<FileDescription>(pipe, O_RDONLY).value();
    auto write_desc = fk::make_ref<FileDescription>(pipe, O_WRONLY).value();

    int fd_read = current_task->add_file_descriptor(read_desc);
    if (fd_read < 0) return static_cast<uint64_t>(fd_read);

    int fd_write = current_task->add_file_descriptor(write_desc);
    if (fd_write < 0) {
        current_task->close_file_descriptor(fd_read);
        return static_cast<uint64_t>(fd_write);
    }

    fk::algorithms::klog("SYSCALL", "sys_pipe: created FDs %d (read) and %d (write)", fd_read, fd_write);

    // TODO: Use a proper validation mechanism for user pointers
    int* user_fds = reinterpret_cast<int*>(pipefd_ptr);
    if (!user_fds) return -14; // -EFAULT

    user_fds[0] = fd_read;
    user_fds[1] = fd_write;

    return 0;
}

}
