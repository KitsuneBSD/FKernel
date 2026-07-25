#pragma once

#include <LibFK/Syscalls/numbers.h>

// Map SYS_* enum values to __NR_* macros for POSIX compatibility
#define __NR_read SYS_READ
#define __NR_write SYS_WRITE
#define __NR_open SYS_OPEN
#define __NR_close SYS_CLOSE
#define __NR_stat SYS_STAT
#define __NR_fstat SYS_FSTAT
#define __NR_lstat SYS_LSTAT
#define __NR_lseek SYS_LSEEK
#define __NR_mmap SYS_MMAP
#define __NR_munmap SYS_MUNMAP
#define __NR_brk SYS_BRK
#define __NR_rt_sigaction SYS_SIGACTION
#define __NR_rt_sigprocmask SYS_SIGPROCMASK
#define __NR_rt_sigreturn SYS_RT_SIGRETURN
#define __NR_ioctl SYS_IOCTL
#define __NR_writev SYS_WRITEV
#define __NR_readv SYS_READV
#define __NR_nanosleep SYS_NANOSLEEP
#define __NR_getpid SYS_GETPID
#define __NR_exit SYS_EXIT
#define __NR_kill SYS_KILL
#define __NR_uname SYS_UNAME
#define __NR_getdents64 SYS_GETDENTS64
#define __NR_getcwd SYS_GETCWD
#define __NR_chdir SYS_CHDIR
#define __NR_mkdir SYS_MKDIR
#define __NR_getuid SYS_GETUID
#define __NR_geteuid SYS_GETEUID
#define __NR_getppid SYS_GETPPID
#define __NR_arch_prctl SYS_ARCH_PRCTL
#define __NR_gettid SYS_GETTID
#define __NR_clock_gettime SYS_CLOCK_GETTIME
#define __NR_exit_group SYS_EXIT_GROUP
#define __NR_pipe SYS_PIPE
#define __NR_dup2 SYS_DUP2
#define __NR_select SYS_SELECT
#define __NR_poll SYS_POLL
#define __NR_access SYS_ACCESS
#define __NR_fcntl SYS_FCNTL
#define __NR_getgid SYS_GETGID
#define __NR_getegid SYS_GETEGID
#define __NR_setuid SYS_SETUID
#define __NR_setgid SYS_SETGID
#define __NR_getgroups SYS_GETGROUPS
#define __NR_setgroups SYS_SETGROUPS
#define __NR_socket SYS_SOCKET
#define __NR_connect SYS_CONNECT
#define __NR_accept SYS_ACCEPT
#define __NR_sendto SYS_SENDTO
#define __NR_recvfrom SYS_RECVFROM
#define __NR_bind SYS_BIND
#define __NR_listen SYS_LISTEN
#define __NR_setsockopt SYS_SETSOCKOPT
#define __NR_getsockopt SYS_GETSOCKOPT
#define __NR_getsockname SYS_GETSOCKNAME
#define __NR_getpeername SYS_GETPEERNAME
#define __NR_shutdown SYS_SHUTDOWN
#define __NR_unlink SYS_UNLINK
#define __NR_rmdir SYS_RMDIR
#define __NR_rename SYS_RENAME
#define __NR_link SYS_LINK
#define __NR_symlink SYS_SYMLINK
#define __NR_readlink SYS_READLINK
#define __NR_chmod SYS_CHMOD
#define __NR_chown SYS_CHOWN
#define __NR_umask SYS_UMASK
#define __NR_mount SYS_MOUNT
#define __NR_umount2 SYS_UMOUNT2
#define __NR_reboot SYS_REBOOT
#define __NR_gettimeofday SYS_GETTIMEOFDAY
#define __NR_getrlimit SYS_GETRLIMIT
#define __NR_getrusage SYS_SYSINFO
#define __NR_fsync SYS_FSYNC
#define __NR_truncate SYS_TRUNCATE
#define __NR_ftruncate SYS_FTRUNCATE
#define __NR_openat SYS_OPENAT
#define __NR_mkdirat SYS_MKDIRAT
#define __NR_fchownat SYS_FCHOWNAT
#define __NR_unlinkat SYS_UNLINKAT
#define __NR_renameat SYS_RENAMEAT
#define __NR_symlinkat SYS_SYMLINKAT
#define __NR_linkat SYS_LINKAT
#define __NR_readlinkat SYS_READLINKAT
#define __NR_fchmodat SYS_FCHMODAT
#define __NR_faccessat SYS_FACCESSAT
#define __NR_newfstatat SYS_NEWFSTATAT
#define __NR_mprotect SYS_MPROTECT
#define __NR_madvise SYS_MADVISE
#define __NR_mremap SYS_MREMAP
#define __NR_msync SYS_MSYNC
#define __NR_mlock SYS_MLOCK
#define __NR_munlock SYS_MUNLOCK
#define __NR_mlockall SYS_MLOCKALL
#define __NR_munlockall SYS_MUNLOCKALL
#define __NR_pread64 SYS_PREAD64
#define __NR_pwrite64 SYS_PWRITE64
#define __NR_dup SYS_DUP
#define __NR_pipe2 SYS_PIPE2
#define __NR_dup3 SYS_DUP3
#define __NR_flock SYS_FLOCK
#define __NR_fchmod SYS_FCHMOD
#define __NR_fchown SYS_FCHOWN
#define __NR_mknod SYS_MKNOD
#define __NR_getrandom SYS_GETRANDOM
#define __NR_fork SYS_FORK
#define __NR_vfork SYS_VFORK
#define __NR_execve SYS_EXECVE
#define __NR_wait4 SYS_WAIT4
#define __NR_setpgid SYS_SETPGID
#define __NR_getpgid SYS_GETPGID
#define __NR_setsid SYS_SETSID
#define __NR_sigaltstack SYS_SIGALTSTACK
#define __NR_tkill SYS_TKILL
#define __NR_tgkill SYS_TGKILL
#define __NR_sigpending SYS_SIGPENDING
#define __NR_sigsuspend SYS_RT_SIGSUSPEND
#define __NR_statfs SYS_STATFS
#define __NR_fstatfs SYS_FSTATFS
#define __NR_prctl SYS_SYSINFO
#define __NR_prlimit64 SYS_PRLIMIT64
#define __NR_epoll_create1 SYS_EPOLL_CREATE1
#define __NR_epoll_ctl SYS_EPOLL_CTL
#define __NR_epoll_wait SYS_EPOLL_WAIT
#define __NR_signalfd SYS_SIGNALFD
#define __NR_signalfd4 SYS_SIGNALFD4
#define __NR_timerfd_create SYS_TIMERFD_CREATE
#define __NR_eventfd SYS_EVENTFD
#define __NR_eventfd2 SYS_EVENTFD2
#define __NR_timerfd_settime SYS_TIMERFD_SETTIME
#define __NR_timerfd_gettime SYS_TIMERFD_GETTIME
#define __NR_kqueue SYS_KQUEUE
#define __NR_kevent SYS_KEVENT

#ifdef __cplusplus
extern "C" {
#endif

long syscall(long number, ...);

#ifdef __cplusplus
}
#endif
