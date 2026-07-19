#pragma once

#ifdef __GLIBC__
#include <signal.h>
#else

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int sig_atomic_t;
typedef uint64_t sigset_t;

// Signal numbers (POSIX)
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
#define SIGSTKFLT 16
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGIO     29
#define SIGPWR    30
#define SIGSYS    31
#define NSIG      32

// SA flags
#define SA_NOCLDSTOP 0x00000001
#define SA_NOCLDWAIT 0x00000002
#define SA_SIGINFO   0x00000004
#define SA_ONSTACK   0x08000000
#define SA_RESTART   0x10000000
#define SA_NODEFER   0x40000000
#define SA_RESETHAND 0x80000000

// Signal dispositions
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

// sigset operations
#define sigemptyset(s)  (*(s) = 0, 0)
#define sigfillset(s)   (*(s) = ~(sigset_t)0, 0)
#define sigaddset(s, n) (*(s) |= (sigset_t)1 << ((n) - 1), 0)
#define sigdelset(s, n) (*(s) &= ~((sigset_t)1 << ((n) - 1)), 0)
#define sigismember(s, n) ((*(s) >> ((n) - 1)) & 1)

// SIG_BLOCK / SIG_UNBLOCK / SIG_SETMASK
#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, void *, void *);
    sigset_t sa_mask;
    int      sa_flags;
    void (*sa_restorer)(void);
};

void (*signal(int sig, void (*handler)(int)))(int);
int kill(int pid, int sig);
int raise(int sig);
int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigsuspend(const sigset_t *mask);
int sigpending(sigset_t *set);

#ifdef __cplusplus
}
#endif

#endif // __GLIBC__
