#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#ifdef __x86_64__
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#endif

SyscallManager &SyscallManager::the() {
  static SyscallManager instance;
  return instance;
}

void SyscallManager::initialize() {
  initialize_syscalls();
  init_syscalls();
}

void SyscallManager::register_syscall(uint64_t num, syscall_function_t fn) {
  if (num < SYS_MAX) {
    m_syscall_table[num] = fn;
  }
}

uint64_t SyscallManager::handle(uint64_t num, uint64_t arg1, uint64_t arg2,
                                uint64_t arg3, uint64_t arg4, uint64_t arg5,
                                uint64_t arg6, PtRegs* regs) {
  if (num >= SYS_MAX || !m_syscall_table[num]) {
    return -38; // -ENOSYS
  }
  return m_syscall_table[num](arg1, arg2, arg3, arg4, arg5, arg6, regs);
}

extern "C" {
uint64_t sys_open(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_close(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_read(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_write(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_writev(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_lseek(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_exit(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_yield(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_nanosleep(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                       uint64_t, PtRegs*);
uint64_t sys_mkdir(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getdents64(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                        uint64_t, PtRegs*);
uint64_t sys_chdir(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fork(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_vfork(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mount(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_umount2(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                     uint64_t, PtRegs*);
uint64_t sys_execve(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_dup2(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_wait4(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mmap(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_munmap(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_brk(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getpid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_gettid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getppid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                     uint64_t, PtRegs*);
uint64_t sys_getuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_geteuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                     uint64_t, PtRegs*);
uint64_t sys_getgid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getegid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                     uint64_t, PtRegs*);
uint64_t sys_getpgrp(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                     uint64_t, PtRegs*);
uint64_t sys_fcntl(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_kill(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sigaction(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                       uint64_t, PtRegs*);
uint64_t sys_sigprocmask(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                         uint64_t, PtRegs*);
uint64_t sys_rt_sigsuspend(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                           uint64_t, PtRegs*);
uint64_t sys_stat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fstat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_lstat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_socket(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_bind(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_connect(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                     uint64_t, PtRegs*);
uint64_t sys_listen(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_accept(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_uname(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_ioctl(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getcwd(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_arch_prctl(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                        uint64_t, PtRegs*);
uint64_t sys_set_tid_address(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                             uint64_t, PtRegs*);
uint64_t sys_clock_gettime(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                           uint64_t, PtRegs*);
uint64_t sys_gettimeofday(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                          uint64_t, PtRegs*);
uint64_t sys_exit_group(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                        uint64_t, PtRegs*);
uint64_t sys_ipc_send(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                      uint64_t, PtRegs*);
uint64_t sys_ipc_receive(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                         uint64_t, PtRegs*);
uint64_t sys_ipc_call(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                      uint64_t, PtRegs*);
uint64_t sys_getdents(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                      uint64_t, PtRegs*);
uint64_t sys_newfstatat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                        uint64_t, PtRegs*);
uint64_t sys_pipe(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_kqueue(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_kevent(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_tty_create_kernel(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_tty_delete_kernel(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_tty_list_kernel(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
}

extern "C" void initialize_syscalls() {
  SyscallManager::the().register_syscall(SYS_OPEN, sys_open);
  SyscallManager::the().register_syscall(SYS_CLOSE, sys_close);
  SyscallManager::the().register_syscall(SYS_READ, sys_read);
  SyscallManager::the().register_syscall(SYS_WRITE, sys_write);
  SyscallManager::the().register_syscall(SYS_WRITEV, sys_writev);
  SyscallManager::the().register_syscall(SYS_LSEEK, sys_lseek);
  SyscallManager::the().register_syscall(SYS_EXIT, sys_exit);
  SyscallManager::the().register_syscall(SYS_YIELD, sys_yield);
  SyscallManager::the().register_syscall(SYS_NANOSLEEP, sys_nanosleep);
  SyscallManager::the().register_syscall(SYS_MKDIR, sys_mkdir);
  SyscallManager::the().register_syscall(SYS_READDIR, sys_getdents);
  SyscallManager::the().register_syscall(SYS_GETDENTS64, sys_getdents64);
  SyscallManager::the().register_syscall(262, sys_newfstatat); // SYS_NEWFSTATAT
  SyscallManager::the().register_syscall(SYS_CHDIR, sys_chdir);
  SyscallManager::the().register_syscall(SYS_CLONE, sys_vfork);
  SyscallManager::the().register_syscall(SYS_FORK, sys_fork);
  SyscallManager::the().register_syscall(SYS_VFORK, sys_vfork);
  SyscallManager::the().register_syscall(SYS_EXECVE, sys_execve);
  SyscallManager::the().register_syscall(SYS_FCNTL, sys_fcntl);
  SyscallManager::the().register_syscall(SYS_PIPE, sys_pipe);
  SyscallManager::the().register_syscall(SYS_DUP2, sys_dup2);
  SyscallManager::the().register_syscall(SYS_WAIT4, sys_wait4);
  SyscallManager::the().register_syscall(SYS_MMAP, sys_mmap);
  SyscallManager::the().register_syscall(SYS_MUNMAP, sys_munmap);
  SyscallManager::the().register_syscall(SYS_BRK, sys_brk);
  SyscallManager::the().register_syscall(SYS_GETPID, sys_getpid);
  SyscallManager::the().register_syscall(SYS_GETTID, sys_gettid);
  SyscallManager::the().register_syscall(SYS_GETPPID, sys_getppid);
  SyscallManager::the().register_syscall(SYS_GETUID, sys_getuid);
  SyscallManager::the().register_syscall(SYS_GETGID, sys_getgid);
  SyscallManager::the().register_syscall(SYS_GETEUID, sys_geteuid);
  SyscallManager::the().register_syscall(SYS_GETEGID, sys_getegid);
  SyscallManager::the().register_syscall(SYS_GETPGRP, sys_getpgrp);
  SyscallManager::the().register_syscall(SYS_KILL, sys_kill);
  SyscallManager::the().register_syscall(SYS_SIGACTION, sys_sigaction);
  SyscallManager::the().register_syscall(SYS_SIGPROCMASK, sys_sigprocmask);
  SyscallManager::the().register_syscall(SYS_RT_SIGSUSPEND, sys_rt_sigsuspend);
  SyscallManager::the().register_syscall(SYS_STAT, sys_stat);
  SyscallManager::the().register_syscall(SYS_FSTAT, sys_fstat);
  SyscallManager::the().register_syscall(SYS_LSTAT, sys_lstat);
  SyscallManager::the().register_syscall(SYS_SOCKET, sys_socket);
  SyscallManager::the().register_syscall(SYS_BIND, sys_bind);
  SyscallManager::the().register_syscall(SYS_CONNECT, sys_connect);
  SyscallManager::the().register_syscall(SYS_LISTEN, sys_listen);
  SyscallManager::the().register_syscall(SYS_ACCEPT, sys_accept);
  SyscallManager::the().register_syscall(SYS_UNAME, sys_uname);
  SyscallManager::the().register_syscall(SYS_IOCTL, sys_ioctl);
  SyscallManager::the().register_syscall(SYS_GETCWD, sys_getcwd);
  SyscallManager::the().register_syscall(SYS_ARCH_PRCTL, sys_arch_prctl);
  SyscallManager::the().register_syscall(SYS_SET_TID_ADDRESS,
                                         sys_set_tid_address);
  SyscallManager::the().register_syscall(SYS_CLOCK_GETTIME, sys_clock_gettime);
  SyscallManager::the().register_syscall(SYS_GETTIMEOFDAY, sys_gettimeofday);
  SyscallManager::the().register_syscall(SYS_EXIT_GROUP, sys_exit_group);
  SyscallManager::the().register_syscall(SYS_IPC_SEND, sys_ipc_send);
  SyscallManager::the().register_syscall(SYS_IPC_RECEIVE, sys_ipc_receive);
  SyscallManager::the().register_syscall(SYS_IPC_CALL, sys_ipc_call);
  SyscallManager::the().register_syscall(SYS_MOUNT, sys_mount);
  SyscallManager::the().register_syscall(SYS_UMOUNT2, sys_umount2);
  SyscallManager::the().register_syscall(SYS_KQUEUE, sys_kqueue);
  SyscallManager::the().register_syscall(SYS_KEVENT, sys_kevent);
  SyscallManager::the().register_syscall(SYS_TTY_CREATE, sys_tty_create_kernel);
  SyscallManager::the().register_syscall(SYS_TTY_DELETE, sys_tty_delete_kernel);
  SyscallManager::the().register_syscall(SYS_TTY_LIST, sys_tty_list_kernel);
}
extern "C" uint64_t syscall_dispatcher(uint64_t num, uint64_t arg1,
                                       uint64_t arg2, uint64_t arg3,
                                       uint64_t arg4, uint64_t arg5,
                                       uint64_t arg6, PtRegs* regs) {
  auto *task = SchedulerManager::the().current();

  uint64_t result =
      SyscallManager::the().handle(num, arg1, arg2, arg3, arg4, arg5, arg6, regs);

  // Log to SyscallLogNode (DebugFS) instead of console
  if (MemoryManager::the().is_heap_initialized()) {
    char log_buf[256];
    int log_len = snprintf(log_buf, sizeof(log_buf),
                           "[SYSCALL] Task %lu: %lu (args: %p, %p, %p) -> %p\n",
                           task ? task->id : 0, num, (void *)arg1, (void *)arg2,
                           (void *)arg3, (void *)result);

    auto syscall_log = fkernel::SyscallLogNode::the();
    if (syscall_log)
      syscall_log->append(log_buf, log_len);
  }

  if (task && !task->is_a_kernel_task) {
    fkernel::ipc::SignalDelivery::handle_pending_signals(task);
  }

  return result;
}
