#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Syscall wrappers (implemented in syscalls.asm)
int64_t sys_write(int fd, const void* buf, size_t count);
int64_t sys_read(int fd, void* buf, size_t count);
int sys_open(const char* path, int flags, ...);
int sys_close(int fd);
void sys_exit(int code);
int sys_mkdir(const char* path, int mode);
int sys_chdir(const char* path);
int sys_getcwd(char* buf, size_t size);
int sys_ioctl(int fd, uint64_t request, void* arg);

// Terminal ioctl commands
#define TIOCSCTTY  0x540E  // Set controlling terminal
#define TIOCGPGRP  0x540F  // Get foreground process group
#define TIOCSPGRP  0x5410  // Set foreground process group

// Keyboard layout ioctl commands (for use with sys_ioctl on /dev/tty0)
#define KBDIO_SETLAYOUT  0x4B01  // arg: 0=US, 1=US_INTL, 2=ABNT2
#define KBDIO_GETLAYOUT  0x4B02  // returns: KeyboardLayout enum
#define KBDIO_SETCOMPOSE 0x4B03  // arg: 0=off, 1=on (dead key compose mode)

// Signal constants
#define SIG_DFL  ((void (*)(int))0)
#define SIG_IGN  ((void (*)(int))1)
#define SIGINT   2
#define SIGQUIT  3
#define SIGKILL  9
#define SIGUSR1  10
#define SIGCHLD  17
#define SIGTSTP  20

struct sigaction {
    void (*sa_handler)(int);
    uint64_t sa_flags;
    void (*sa_restorer)(void);
    uint64_t sa_mask;
};

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t  d_off;
    uint16_t d_reclen;
    uint8_t  d_type;
    char     d_name[];
};

int64_t sys_getdents64(int fd, void* dirp, size_t count);

// New syscalls
int sys_fork();
int sys_execve(const char* path, char* const argv[], char* const envp[]);
int sys_dup2(int oldfd, int newfd);
int sys_wait4(int pid, int* status, int options, void* rusage);
void* sys_mmap(void* addr, size_t length, int prot, int flags, int fd, long offset);
int sys_munmap(void* addr, size_t length);
void* sys_brk(void* addr);
int sys_getpid();
int sys_getppid();
int sys_getuid();
int sys_getgid();
int sys_geteuid();
int sys_getegid();
int sys_kill(int pid, int sig);
int sys_sigaction(int signum, const void* act, void* oldact);
int sys_sigprocmask(int how, const void* set, void* oldset);
int sys_stat(const char* path, void* buf);
int sys_fstat(int fd, void* buf);
int sys_lstat(const char* path, void* buf);
int sys_socket(int domain, int type, int protocol);
int sys_bind(int sockfd, const void* addr, int addrlen);
int sys_connect(int sockfd, const void* addr, int addrlen);
int sys_listen(int sockfd, int backlog);
int sys_accept(int sockfd, void* addr, int* addrlen);
int sys_uname(void* buf);
int sys_gettimeofday(void* tv, void* tz);
struct timespec {
    long tv_sec;
    long tv_nsec;
};

int sys_nanosleep(const struct timespec* req, struct timespec* rem);
int sys_mount(const char* source, const char* target, const char* filesystemtype, unsigned long mountflags, const void* data);
int sys_umount(const char* target, int flags);
int sys_setsid(void);
int sys_setpgid(int pid, int pgid);
int sys_getpgid(int pid);
int sys_chmod(const char* path, int mode);
int sys_fchmod(int fd, int mode);
int sys_access(const char* path, int mode);
int sys_utimensat(int dirfd, const char* path, const struct timespec times[2], int flags);
int sys_fcntl(int fd, int cmd, long arg);

// basic LibC-style functions
static inline void print(const char* str) {
    if (!str) return;
    size_t len = 0;
    while(str[len]) len++;
    sys_write(1, str, len);
}

#ifdef __cplusplus
}
#endif
