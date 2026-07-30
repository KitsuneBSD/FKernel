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

#define NSIG       65   // 64 signals + sentinel; supports SIGRTMIN(34)..SIGRTMAX(64)
#define SIGRTMIN   34
#define SIGRTMAX   64

#define SI_MAX_SIZE 128

typedef struct {
    int      si_signo;
    int      si_errno;
    int      si_code;
    int      __pad0;
    union {
        int    _pad[(SI_MAX_SIZE - 16) / sizeof(int)];
        struct {
            uint32_t si_pid;
            uint32_t si_uid;
        } _kill;
        struct {
            uint32_t si_tid;
            uint32_t si_overrun;
            uint64_t si_sigval;
        } _timer;
        struct {
            uint32_t si_pid;
            uint32_t si_uid;
            uint64_t si_sigval;
        } _rt;
        struct {
            uint32_t si_pid;
            uint32_t si_uid;
            int      si_status;
            uint64_t si_utime;
            uint64_t si_stime;
        } _sigchld;
        struct {
            uint64_t si_addr;
        } _sigfault;
        struct {
            int32_t  si_band;
            int32_t  si_fd;
        } _sigpoll;
    } __attribute__((aligned(8))) _sifields;
} __attribute__((aligned(8))) siginfo_t;

#define si_pid     _sifields._kill.si_pid
#define si_uid     _sifields._kill.si_uid
#define si_addr    _sifields._sigfault.si_addr
#define si_status  _sifields._sigchld.si_status
#define si_utime   _sifields._sigchld.si_utime
#define si_stime   _sifields._sigchld.si_stime

#define SI_USER     0
#define SI_KERNEL   0x80
#define SI_QUEUE   -1
#define SI_TIMER   -2
#define SI_TKILL   -6

#define ILL_ILLOPC  1
#define ILL_PRVOPC  2
#define FPE_INTDIV  1
#define FPE_INTOVF  2
#define FPE_FLTDIV  3
#define SEGV_MAPERR 1
#define BUS_ADRALN  1
#define BUS_ADRERR  2
#define TRAP_BRKPT  1
#define TRAP_TRACE  2

typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

#define SA_NOCLDSTOP  0x00000001UL
#define SA_NOCLDWAIT  0x00000002UL
#define SA_SIGINFO    0x00000004UL
#define SA_ONSTACK    0x08000000UL
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

static_assert(sizeof(siginfo_t) == 128, "siginfo_t must be 128 bytes");
static_assert(sizeof(struct sigaction) == 32, "sigaction must be 32 bytes");
