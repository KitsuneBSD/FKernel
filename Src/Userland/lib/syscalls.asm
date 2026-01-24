[BITS 64]
%include "Src/Userland/lib/syscalls.inc"

global sys_write
global sys_read
global sys_open
global sys_close
global sys_exit
global sys_mkdir
global sys_chdir
global sys_getcwd
global sys_ioctl
global sys_readdir
global sys_fork
global sys_execve
global sys_dup2
global sys_wait4
global sys_mmap
global sys_munmap
global sys_brk
global sys_getpid
global sys_getppid
global sys_getuid
global sys_geteuid
global sys_kill
global sys_sigaction
global sys_sigprocmask
global sys_stat
global sys_fstat
global sys_lstat
global sys_socket
global sys_bind
global sys_connect
global sys_listen
global sys_accept
global sys_uname
global sys_getdents64

section .text

; int64_t sys_getdents64(int fd, void* dirp, size_t count)
sys_getdents64:
    mov rax, SYS_GETDENTS64
    syscall
    ret

; uint64_t sys_write(int fd, const void* buf, size_t count)
sys_write:
    mov rax, SYS_WRITE
    syscall
    ret

; uint64_t sys_read(int fd, void* buf, size_t count)
sys_read:
    mov rax, SYS_READ
    syscall
    ret

; void sys_exit(int code)
sys_exit:
    mov rax, SYS_EXIT
    syscall
    ret

; int sys_mkdir(const char* path, int mode)
sys_mkdir:
    mov rax, SYS_MKDIR
    syscall
    ret

; int sys_readdir(const char* path, void* buf, int max_count)
sys_readdir:
    mov rax, SYS_READDIR
    syscall
    ret

; pid_t sys_fork()
sys_fork:
    mov rax, SYS_FORK
    syscall
    ret

; int sys_execve(const char* path, char* const argv[], char* const envp[])
sys_execve:
    mov rax, SYS_EXECVE
    syscall
    ret

; int sys_dup2(int oldfd, int newfd)
sys_dup2:
    mov rax, SYS_DUP2
    syscall
    ret

; pid_t sys_wait4(pid_t pid, int *wstatus, int options, struct rusage *rusage)
sys_wait4:
    mov rax, SYS_WAIT4
    syscall
    ret

; void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
sys_mmap:
    mov rax, SYS_MMAP
    syscall
    ret

; int sys_munmap(void *addr, size_t length)
sys_munmap:
    mov rax, SYS_MUNMAP
    syscall
    ret

; void *sys_brk(void *addr)
sys_brk:
    mov rax, SYS_BRK
    syscall
    ret

; pid_t sys_getpid(void)
sys_getpid:
    mov rax, SYS_GETPID
    syscall
    ret

; pid_t sys_getppid(void)
sys_getppid:
    mov rax, SYS_GETPPID
    syscall
    ret

; uid_t sys_getuid(void)
sys_getuid:
    mov rax, SYS_GETUID
    syscall
    ret

; uid_t sys_geteuid(void)
sys_geteuid:
    mov rax, SYS_GETEUID
    syscall
    ret

; int sys_kill(pid_t pid, int sig)
sys_kill:
    mov rax, SYS_KILL
    syscall
    ret

; int sys_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
sys_sigaction:
    mov rax, SYS_SIGACTION
    syscall
    ret

; int sys_sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
sys_sigprocmask:
    mov rax, SYS_SIGPROCMASK
    syscall
    ret

; int sys_stat(const char *pathname, struct stat *statbuf)
sys_stat:
    mov rax, SYS_STAT
    syscall
    ret

; int sys_fstat(int fd, struct stat *statbuf)
sys_fstat:
    mov rax, SYS_FSTAT
    syscall
    ret

; int sys_stat(const char *pathname, struct stat *statbuf)
sys_lstat:
    mov rax, SYS_LSTAT
    syscall
    ret

; int sys_socket(int domain, int type, int protocol)
sys_socket:
    mov rax, SYS_SOCKET
    syscall
    ret

; int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
sys_bind:
    mov rax, SYS_BIND
    syscall
    ret

; int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
sys_connect:
    mov rax, SYS_CONNECT
    syscall
    ret

; int sys_listen(int sockfd, int backlog)
sys_listen:
    mov rax, SYS_LISTEN
    syscall
    ret

; int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)

sys_accept:

    mov rax, SYS_ACCEPT

    syscall

    ret



; int sys_uname(struct utsname *buf)

sys_uname:

    mov rax, SYS_UNAME

    syscall

    ret



sys_open:

    mov rax, SYS_OPEN

    syscall

    ret



sys_close:

    mov rax, SYS_CLOSE

    syscall

    ret



sys_chdir:

    mov rax, SYS_CHDIR

    syscall

    ret



sys_getcwd:

    mov rax, SYS_GETCWD

    syscall

    ret



sys_ioctl:

    mov rax, SYS_IOCTL

    syscall

    ret
