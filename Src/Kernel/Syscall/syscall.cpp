#include <Kernel/Fs/Virtual/DebugFs/debug_fs.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/Logging/log.h>
#ifdef __x86_64__
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#endif


SyscallManager& SyscallManager::the() {
  static SyscallManager instance;
  return instance;
}

void SyscallManager::initialize() {
  initialize_syscalls();
  init_syscalls(0);
  fk::algorithms::klog("SYSCALL", "Syscall manager initialized");
  m_is_initialized = true;
}

void SyscallManager::register_syscall(uint64_t num, syscall_function_t fn) {
  if (num < SYS_MAX) {
    m_syscall_table[num] = fn;
  }
}

uint64_t SyscallManager::handle(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                uint64_t arg4, uint64_t arg5, uint64_t arg6, PtRegs* regs) {
  if (num >= SYS_MAX || !m_syscall_table[num]) {
    fk::algorithms::kwarn("SYSCALL", "Unknown syscall %lu", num);
    return return_error(fk::core::Error::NotImplemented);
  }
  auto* task = SchedulerManager::the().current();
  if (task && task->control.identity.id.value() == 1) {
    fk::algorithms::kdebug("PID1", "syscall #%lu", num);
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
uint64_t sys_nanosleep(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mkdir(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_rmdir(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_unlink(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_creat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_link(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_rename(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getdents64(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_chdir(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fork(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_vfork(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_clone(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mount(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_pivot_root(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_umount2(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_execve(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_dup(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_dup2(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_wait4(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mmap(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_munmap(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_brk(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getpid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_gettid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getppid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_geteuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getgid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getegid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getpgrp(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fcntl(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_kill(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sigaction(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sigprocmask(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sigreturn(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_rt_sigsuspend(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_stat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fstat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_lstat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_socket(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_bind(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_connect(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_listen(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_accept(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sendto(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_recvfrom(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sendmsg(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_recvmsg(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_shutdown(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getsockname(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getpeername(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_socketpair(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setsockopt(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getsockopt(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_signalfd(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_timerfd_create(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_eventfd(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_timerfd_settime(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_timerfd_gettime(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_signalfd4(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_eventfd2(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_uname(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_ioctl(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getcwd(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_arch_prctl(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_set_tid_address(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_clock_gettime(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_gettimeofday(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_exit_group(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_ipc_send(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_ipc_receive(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_ipc_call(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_cap_revoke(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_cap_transfer(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_cap_grant(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_bind_irq(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_unbind_irq(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_openpty(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getdents(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_newfstatat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_pipe(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_pipe2(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_dup3(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mprotect(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setsid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_poll(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_select(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_kqueue(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_kevent(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_tty_create_kernel(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_tty_delete_kernel(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_tty_list_kernel(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_access(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_chmod(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_chown(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mknod(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mkfifo(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sem_open(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sem_wait(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sem_post(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sem_getvalue(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sem_unlink(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mq_open(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mq_send(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mq_receive(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mq_unlink(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_shm_open(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_shm_unlink(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_symlink(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_readlink(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setgid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setreuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setregid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setresuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getresuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setresgid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getresgid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sigpending(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_madvise(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getrandom(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_futex(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_prlimit64(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sched_getscheduler(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sched_setscheduler(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sched_getparam(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sched_setparam(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sched_get_priority_max(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sched_get_priority_min(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_timer_create(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_timer_delete(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_timer_settime(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_timer_gettime(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_timer_getoverrun(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setpgid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getpgid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getgroups(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setgroups(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getrlimit(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setrlimit(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_truncate(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_ftruncate(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fsync(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_openat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mkdirat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mknodat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fchmod(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fchown(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fchownat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_unlinkat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_renameat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_symlinkat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_linkat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_readlinkat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fchmodat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_faccessat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_chroot(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_umask(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_readv(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_pread64(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_pwrite64(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_flock(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getitimer(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setitimer(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_nice(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_getpriority(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_setpriority(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_thread_set_qos_class(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_thread_get_qos_class(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_clock_getres(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_clock_nanosleep(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sendfile(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_statfs(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_fstatfs(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mlock(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_munlock(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mlockall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_munlockall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_msync(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_mremap(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sysinfo(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_syslog(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_sigaltstack(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_tgkill(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_tkill(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_epoll_create1(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_epoll_ctl(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_epoll_wait(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_rt_sigtimedwait(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_reboot(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_accept4(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_utimensat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_utimes(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_futimesat(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_personality(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_prctl(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_set_robust_list(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
uint64_t sys_get_robust_list(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
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
  SyscallManager::the().register_syscall(SYS_RMDIR, sys_rmdir);
  SyscallManager::the().register_syscall(SYS_UNLINK, sys_unlink);
  SyscallManager::the().register_syscall(SYS_CREAT, sys_creat);
  SyscallManager::the().register_syscall(SYS_LINK, sys_link);
  SyscallManager::the().register_syscall(SYS_RENAME, sys_rename);
  SyscallManager::the().register_syscall(SYS_GETDENTS, sys_getdents);
  SyscallManager::the().register_syscall(SYS_GETDENTS64, sys_getdents64);
  SyscallManager::the().register_syscall(SYS_NEWFSTATAT, sys_newfstatat);
  SyscallManager::the().register_syscall(SYS_CHDIR, sys_chdir);
  SyscallManager::the().register_syscall(SYS_CLONE, sys_clone);
  SyscallManager::the().register_syscall(SYS_FORK, sys_fork);
  SyscallManager::the().register_syscall(SYS_VFORK, sys_vfork);
  SyscallManager::the().register_syscall(SYS_EXECVE, sys_execve);
  SyscallManager::the().register_syscall(SYS_FCNTL, sys_fcntl);
  SyscallManager::the().register_syscall(SYS_PIPE, sys_pipe);
  SyscallManager::the().register_syscall(SYS_PIPE2, sys_pipe2);
  SyscallManager::the().register_syscall(SYS_DUP, sys_dup);
  SyscallManager::the().register_syscall(SYS_DUP2, sys_dup2);
  SyscallManager::the().register_syscall(SYS_DUP3, sys_dup3);
  SyscallManager::the().register_syscall(SYS_WAIT4, sys_wait4);
  SyscallManager::the().register_syscall(SYS_MPROTECT, sys_mprotect);
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
  SyscallManager::the().register_syscall(SYS_SIGRETURN, sys_sigreturn);
  SyscallManager::the().register_syscall(SYS_RT_SIGSUSPEND, sys_rt_sigsuspend);
  SyscallManager::the().register_syscall(SYS_STAT, sys_stat);
  SyscallManager::the().register_syscall(SYS_FSTAT, sys_fstat);
  SyscallManager::the().register_syscall(SYS_LSTAT, sys_lstat);
  SyscallManager::the().register_syscall(SYS_SOCKET, sys_socket);
  SyscallManager::the().register_syscall(SYS_BIND, sys_bind);
  SyscallManager::the().register_syscall(SYS_CONNECT, sys_connect);
  SyscallManager::the().register_syscall(SYS_LISTEN, sys_listen);
  SyscallManager::the().register_syscall(SYS_ACCEPT, sys_accept);
  SyscallManager::the().register_syscall(SYS_ACCEPT4, sys_accept4);
  SyscallManager::the().register_syscall(SYS_SENDTO, sys_sendto);
  SyscallManager::the().register_syscall(SYS_RECVFROM, sys_recvfrom);
  SyscallManager::the().register_syscall(SYS_SENDMSG, sys_sendmsg);
  SyscallManager::the().register_syscall(SYS_RECVMSG, sys_recvmsg);
  SyscallManager::the().register_syscall(SYS_SHUTDOWN, sys_shutdown);
  SyscallManager::the().register_syscall(SYS_GETSOCKNAME, sys_getsockname);
  SyscallManager::the().register_syscall(SYS_GETPEERNAME, sys_getpeername);
  SyscallManager::the().register_syscall(SYS_SOCKETPAIR, sys_socketpair);
  SyscallManager::the().register_syscall(SYS_SETSOCKOPT, sys_setsockopt);
  SyscallManager::the().register_syscall(SYS_GETSOCKOPT, sys_getsockopt);
  SyscallManager::the().register_syscall(SYS_SIGNALFD, sys_signalfd);
  SyscallManager::the().register_syscall(SYS_TIMERFD_CREATE, sys_timerfd_create);
  SyscallManager::the().register_syscall(SYS_EVENTFD, sys_eventfd);
  SyscallManager::the().register_syscall(SYS_TIMERFD_SETTIME, sys_timerfd_settime);
  SyscallManager::the().register_syscall(SYS_TIMERFD_GETTIME, sys_timerfd_gettime);
  SyscallManager::the().register_syscall(SYS_SIGNALFD4, sys_signalfd4);
  SyscallManager::the().register_syscall(SYS_EVENTFD2, sys_eventfd2);
  SyscallManager::the().register_syscall(SYS_UNAME, sys_uname);
  SyscallManager::the().register_syscall(SYS_IOCTL, sys_ioctl);
  SyscallManager::the().register_syscall(SYS_GETCWD, sys_getcwd);
  SyscallManager::the().register_syscall(SYS_ARCH_PRCTL, sys_arch_prctl);
  SyscallManager::the().register_syscall(SYS_SET_TID_ADDRESS, sys_set_tid_address);
  SyscallManager::the().register_syscall(SYS_CLOCK_GETTIME, sys_clock_gettime);
  SyscallManager::the().register_syscall(SYS_GETTIMEOFDAY, sys_gettimeofday);
  SyscallManager::the().register_syscall(SYS_EXIT_GROUP, sys_exit_group);
  SyscallManager::the().register_syscall(SYS_IPC_SEND, sys_ipc_send);
  SyscallManager::the().register_syscall(SYS_IPC_RECEIVE, sys_ipc_receive);
  SyscallManager::the().register_syscall(SYS_IPC_CALL, sys_ipc_call);
  SyscallManager::the().register_syscall(SYS_CAP_REVOKE, sys_cap_revoke);
  SyscallManager::the().register_syscall(SYS_CAP_TRANSFER, sys_cap_transfer);
  SyscallManager::the().register_syscall(SYS_CAP_GRANT, sys_cap_grant);
  SyscallManager::the().register_syscall(SYS_BIND_IRQ, sys_bind_irq);
  SyscallManager::the().register_syscall(SYS_UNBIND_IRQ, sys_unbind_irq);
  SyscallManager::the().register_syscall(SYS_MOUNT, sys_mount);
  SyscallManager::the().register_syscall(SYS_UMOUNT2, sys_umount2);
  SyscallManager::the().register_syscall(SYS_PIVOT_ROOT, sys_pivot_root);
  SyscallManager::the().register_syscall(SYS_POLL, sys_poll);
  SyscallManager::the().register_syscall(SYS_SELECT, sys_select);
  SyscallManager::the().register_syscall(SYS_KQUEUE, sys_kqueue);
  SyscallManager::the().register_syscall(SYS_KEVENT, sys_kevent);
  SyscallManager::the().register_syscall(SYS_TTY_CREATE, sys_tty_create_kernel);
  SyscallManager::the().register_syscall(SYS_TTY_DELETE, sys_tty_delete_kernel);
  SyscallManager::the().register_syscall(SYS_TTY_LIST, sys_tty_list_kernel);
  SyscallManager::the().register_syscall(SYS_ACCESS, sys_access);
  SyscallManager::the().register_syscall(SYS_CHMOD, sys_chmod);
  SyscallManager::the().register_syscall(SYS_FCHMOD, sys_fchmod);
  SyscallManager::the().register_syscall(SYS_CHOWN, sys_chown);
  SyscallManager::the().register_syscall(SYS_FCHOWN, sys_fchown);
  SyscallManager::the().register_syscall(SYS_MKNOD, sys_mknod);
  SyscallManager::the().register_syscall(SYS_MKFIFO, sys_mkfifo);
  SyscallManager::the().register_syscall(SYS_SEM_OPEN, sys_sem_open);
  SyscallManager::the().register_syscall(SYS_SEM_WAIT, sys_sem_wait);
  SyscallManager::the().register_syscall(SYS_SEM_POST, sys_sem_post);
  SyscallManager::the().register_syscall(SYS_SEM_GETVALUE, sys_sem_getvalue);
  SyscallManager::the().register_syscall(SYS_SEM_UNLINK, sys_sem_unlink);
  SyscallManager::the().register_syscall(SYS_MQ_OPEN, sys_mq_open);
  SyscallManager::the().register_syscall(SYS_MQ_SEND, sys_mq_send);
  SyscallManager::the().register_syscall(SYS_MQ_RECEIVE, sys_mq_receive);
  SyscallManager::the().register_syscall(SYS_MQ_UNLINK, sys_mq_unlink);
  SyscallManager::the().register_syscall(SYS_SHM_OPEN, sys_shm_open);
  SyscallManager::the().register_syscall(SYS_SHM_UNLINK, sys_shm_unlink);
  SyscallManager::the().register_syscall(SYS_SYMLINK, sys_symlink);
  SyscallManager::the().register_syscall(SYS_READLINK, sys_readlink);
  SyscallManager::the().register_syscall(SYS_SETUID, sys_setuid);
  SyscallManager::the().register_syscall(SYS_SETGID, sys_setgid);
  SyscallManager::the().register_syscall(SYS_SETREUID, sys_setreuid);
  SyscallManager::the().register_syscall(SYS_SETREGID, sys_setregid);
  SyscallManager::the().register_syscall(SYS_SETRESUID, sys_setresuid);
  SyscallManager::the().register_syscall(SYS_GETRESUID, sys_getresuid);
  SyscallManager::the().register_syscall(SYS_SETRESGID, sys_setresgid);
  SyscallManager::the().register_syscall(SYS_GETRESGID, sys_getresgid);
  SyscallManager::the().register_syscall(SYS_SIGPENDING, sys_sigpending);
  SyscallManager::the().register_syscall(SYS_MADVISE, sys_madvise);
  SyscallManager::the().register_syscall(SYS_GETRANDOM, sys_getrandom);
  SyscallManager::the().register_syscall(SYS_FUTEX, sys_futex);
  SyscallManager::the().register_syscall(SYS_PRLIMIT64, sys_prlimit64);
  SyscallManager::the().register_syscall(SYS_SCHED_GETSCHEDULER, sys_sched_getscheduler);
  SyscallManager::the().register_syscall(SYS_SCHED_SETSCHEDULER, sys_sched_setscheduler);
  SyscallManager::the().register_syscall(SYS_SCHED_GETPARAM, sys_sched_getparam);
  SyscallManager::the().register_syscall(SYS_SCHED_SETPARAM, sys_sched_setparam);
  SyscallManager::the().register_syscall(SYS_SCHED_GET_PRIORITY_MAX, sys_sched_get_priority_max);
  SyscallManager::the().register_syscall(SYS_SCHED_GET_PRIORITY_MIN, sys_sched_get_priority_min);
  SyscallManager::the().register_syscall(SYS_TIMER_CREATE, sys_timer_create);
  SyscallManager::the().register_syscall(SYS_TIMER_DELETE, sys_timer_delete);
  SyscallManager::the().register_syscall(SYS_TIMER_SETTIME, sys_timer_settime);
  SyscallManager::the().register_syscall(SYS_TIMER_GETTIME, sys_timer_gettime);
  SyscallManager::the().register_syscall(SYS_TIMER_GETOVERRUN, sys_timer_getoverrun);
  SyscallManager::the().register_syscall(SYS_SETSID, sys_setsid);
  SyscallManager::the().register_syscall(SYS_SETPGID, sys_setpgid);
  SyscallManager::the().register_syscall(SYS_GETPGID, sys_getpgid);
  SyscallManager::the().register_syscall(SYS_GETGROUPS, sys_getgroups);
  SyscallManager::the().register_syscall(SYS_SETGROUPS, sys_setgroups);
  SyscallManager::the().register_syscall(SYS_GETRLIMIT, sys_getrlimit);
  SyscallManager::the().register_syscall(SYS_SETRLIMIT, sys_setrlimit);
  SyscallManager::the().register_syscall(SYS_TRUNCATE, sys_truncate);
  SyscallManager::the().register_syscall(SYS_FTRUNCATE, sys_ftruncate);
  SyscallManager::the().register_syscall(SYS_FSYNC, sys_fsync);
  SyscallManager::the().register_syscall(SYS_OPENPTY, sys_openpty);
  SyscallManager::the().register_syscall(SYS_OPENAT, sys_openat);
  SyscallManager::the().register_syscall(SYS_MKDIRAT, sys_mkdirat);
  SyscallManager::the().register_syscall(SYS_MKNODAT, sys_mknodat);
  SyscallManager::the().register_syscall(SYS_FCHOWNAT, sys_fchownat);
  SyscallManager::the().register_syscall(SYS_UNLINKAT, sys_unlinkat);
  SyscallManager::the().register_syscall(SYS_RENAMEAT, sys_renameat);
  SyscallManager::the().register_syscall(SYS_SYMLINKAT, sys_symlinkat);
  SyscallManager::the().register_syscall(SYS_LINKAT, sys_linkat);
  SyscallManager::the().register_syscall(SYS_READLINKAT, sys_readlinkat);
  SyscallManager::the().register_syscall(SYS_FCHMODAT, sys_fchmodat);
  SyscallManager::the().register_syscall(SYS_FACCESSAT, sys_faccessat);
  SyscallManager::the().register_syscall(SYS_CHROOT, sys_chroot);
  SyscallManager::the().register_syscall(SYS_UMASK, sys_umask);
  SyscallManager::the().register_syscall(SYS_READV, sys_readv);
  SyscallManager::the().register_syscall(SYS_PREAD64, sys_pread64);
  SyscallManager::the().register_syscall(SYS_PWRITE64, sys_pwrite64);
  SyscallManager::the().register_syscall(SYS_FLOCK, sys_flock);
  SyscallManager::the().register_syscall(SYS_GETITIMER, sys_getitimer);
  SyscallManager::the().register_syscall(SYS_SETITIMER, sys_setitimer);
  SyscallManager::the().register_syscall(SYS_NICE, sys_nice);
  SyscallManager::the().register_syscall(SYS_GETPRIORITY, sys_getpriority);
  SyscallManager::the().register_syscall(SYS_SETPRIORITY, sys_setpriority);
  SyscallManager::the().register_syscall(SYS_THREAD_SET_QOS_CLASS, sys_thread_set_qos_class);
  SyscallManager::the().register_syscall(SYS_THREAD_GET_QOS_CLASS, sys_thread_get_qos_class);
  SyscallManager::the().register_syscall(SYS_CLOCK_GETRES, sys_clock_getres);
  SyscallManager::the().register_syscall(SYS_CLOCK_NANOSLEEP, sys_clock_nanosleep);
  SyscallManager::the().register_syscall(SYS_SENDFILE, sys_sendfile);
  SyscallManager::the().register_syscall(SYS_STATFS, sys_statfs);
  SyscallManager::the().register_syscall(SYS_FSTATFS, sys_fstatfs);
  SyscallManager::the().register_syscall(SYS_MLOCK, sys_mlock);
  SyscallManager::the().register_syscall(SYS_MUNLOCK, sys_munlock);
  SyscallManager::the().register_syscall(SYS_MLOCKALL, sys_mlockall);
  SyscallManager::the().register_syscall(SYS_MUNLOCKALL, sys_munlockall);
  SyscallManager::the().register_syscall(SYS_MSYNC, sys_msync);
  SyscallManager::the().register_syscall(SYS_MREMAP, sys_mremap);
  SyscallManager::the().register_syscall(SYS_SYSINFO, sys_sysinfo);
  SyscallManager::the().register_syscall(SYS_SYSLOG, sys_syslog);
  SyscallManager::the().register_syscall(SYS_SIGALTSTACK, sys_sigaltstack);
  SyscallManager::the().register_syscall(SYS_TGKILL, sys_tgkill);
  SyscallManager::the().register_syscall(SYS_TKILL, sys_tkill);
  SyscallManager::the().register_syscall(SYS_EPOLL_CREATE1, sys_epoll_create1);
  SyscallManager::the().register_syscall(SYS_EPOLL_CTL, sys_epoll_ctl);
  SyscallManager::the().register_syscall(SYS_EPOLL_WAIT, sys_epoll_wait);
  SyscallManager::the().register_syscall(SYS_RT_SIGTIMEDWAIT, sys_rt_sigtimedwait);
  SyscallManager::the().register_syscall(SYS_REBOOT, sys_reboot);
  SyscallManager::the().register_syscall(SYS_UTIMENSAT, sys_utimensat);
  SyscallManager::the().register_syscall(SYS_UTIMES, sys_utimes);
  SyscallManager::the().register_syscall(SYS_FUTIMESAT, sys_futimesat);
  SyscallManager::the().register_syscall(SYS_PERSONALITY, sys_personality);
  SyscallManager::the().register_syscall(SYS_PRCTL, sys_prctl);
  SyscallManager::the().register_syscall(SYS_SET_ROBUST_LIST, sys_set_robust_list);
  SyscallManager::the().register_syscall(SYS_GET_ROBUST_LIST, sys_get_robust_list);
}
extern "C" uint64_t syscall_dispatcher(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                       uint64_t arg4, uint64_t arg5, uint64_t arg6, PtRegs* regs) {
  auto* task = SchedulerManager::the().current();

  // Log BEFORE handle() so blocking syscalls that never return are visible in dmesg
  if (MemoryManager::the().is_heap_initialized()) {
    char enter_buf[512];
    int enter_len =
        snprintf(enter_buf, sizeof(enter_buf),
                 "[SYSCALL >] Task %lu (%s): %lu (args: %p, %p, %p, %p, %p, %p)\n",
                 task ? task->control.identity.id.value() : 0,
                 task ? task->control.identity.name.c_str() : "unknown", num, (void*)arg1,
                 (void*)arg2, (void*)arg3, (void*)arg4, (void*)arg5, (void*)arg6);
    auto syscall_log = fkernel::SyscallLogNode::the();
    if (syscall_log)
      syscall_log->append(enter_buf, enter_len);
  }

  uint64_t result = SyscallManager::the().handle(num, arg1, arg2, arg3, arg4, arg5, arg6, regs);

  if (MemoryManager::the().is_heap_initialized()) {
    char exit_buf[128];
    int exit_len =
        snprintf(exit_buf, sizeof(exit_buf),
                 "[SYSCALL <] Task %lu (%s): %lu -> %p\n",
                 task ? task->control.identity.id.value() : 0,
                 task ? task->control.identity.name.c_str() : "unknown", num, (void*)result);
    auto syscall_log = fkernel::SyscallLogNode::the();
    if (syscall_log)
      syscall_log->append(exit_buf, exit_len);
  }

  if (task && !task->is_a_kernel_task()) {
    // PtRegs.rax holds the syscall NUMBER at entry (not the return value). Update it now so
    // that a signal frame saved by install_handler_frame captures the correct return value.
    // This makes sigreturn correctly restore the interrupted syscall's result (e.g. -EINTR).
    // Save the number first so SA_RESTART can re-arm it in the signal frame.
    uint64_t orig_syscall_num = regs->rax;
    regs->rax = result;

    fkernel::ipc::SignalDelivery::handle_pending_signals(task, regs, orig_syscall_num);

    // Sync the GS-based return registers from PtRegs. sysretq uses [gs:16]/[gs:24]/[gs:8]
    // directly (not PtRegs), so any modification to regs->rip/rflags/rsp (e.g. by
    // install_handler_frame for signal delivery, or by execve) must be reflected here.
    current_cpu_block().saved_rip    = regs->rip;
    current_cpu_block().saved_rflags = regs->rflags;
    current_cpu_block().user_rsp     = regs->rsp;
  }

  return result;
}
