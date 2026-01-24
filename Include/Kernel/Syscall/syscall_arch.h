#pragma once

#include <Kernel/Syscall/syscall_numbers.h>

// Macros para compatibilidade com código que espera nomes do Linux ou genéricos
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
