#pragma once
#include <LibFK/Types/types.h>

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGURG    16
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGWINCH  28

#define NSIG 32

typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

// Must match the Linux rt_sigaction kernel struct layout (syscall 13):
//   offset  0: sa_handler  (8 bytes)
//   offset  8: sa_flags    (8 bytes)
//   offset 16: sa_restorer (8 bytes)
//   offset 24: sa_mask     (8 bytes)
#define SA_NOCLDSTOP  0x00000001UL
#define SA_NOCLDWAIT  0x00000002UL
#define SA_SIGINFO    0x00000004UL
#define SA_RESTORER   0x04000000UL
#define SA_RESTART    0x10000000UL
#define SA_NODEFER    0x40000000UL
#define SA_RESETHAND  0x80000000UL

struct sigaction {
    sighandler_t sa_handler;
    uint64_t sa_flags;
    void (*sa_restorer)(void);
    uint64_t sa_mask;
};
