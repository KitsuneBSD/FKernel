#include <fk_user.h>

#define MAX_ARGS 32
#define MAX_LINE 512
#define MAX_PATH 256

static int fk_strlen(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}

static void fk_puts(const char* s) {
    sys_write(1, s, fk_strlen(s));
}

static int fk_strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void fk_strncpy(char* dst, const char* src, int n) {
    int i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static void setup_signals(void* handler) {
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = 0;
    act.sa_restorer = 0;
    act.sa_mask = 0;
    sys_sigaction(SIGINT, &act, 0);
    sys_sigaction(SIGQUIT, &act, 0);
    sys_sigaction(SIGTSTP, &act, 0);
}

static int fk_strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

static int read_line(char* buf, int max) {
    int n = 0;
    char c;
    while (n < max - 1) {
        int r = sys_read(0, &c, 1);
        if (r <= 0) {
            if (n == 0) return -1;
            break;
        }
        if (c == '\n') break;
        if (c == '\r') continue;
        if (c == '\b' || c == 0x7f) {
            if (n > 0) {
                n--;
                sys_write(1, "\b \b", 3);
            }
            continue;
        }
        sys_write(1, &c, 1);
        buf[n++] = c;
    }
    buf[n] = 0;
    sys_write(1, "\n", 1);
    return n;
}

static int parse_args(char* line, char* args[], int max) {
    int argc = 0;
    char* p = line;
    while (*p && argc < max - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        args[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) { *p = 0; p++; }
    }
    args[argc] = 0;
    return argc;
}

static void exec_command(char** args, char** envp) {
    sys_execve(args[0], args, envp);

    // Retry with /bin/ prefix if not found
    if (args[0][0] != '/') {
        char full[MAX_PATH];
        const char* prefix = "/bin/";
        int plen = fk_strlen(prefix);
        int alen = fk_strlen(args[0]);
        if (plen + alen < MAX_PATH) {
            for (int i = 0; i < plen; i++) full[i] = prefix[i];
            for (int i = 0; i <= alen; i++) full[plen + i] = args[0][i];
            args[0] = full;
            sys_execve(full, args, envp);
        }
    }

    fk_puts("sh: command not found: ");
    fk_puts(args[0]);
    fk_puts("\n");
    sys_exit(127);
}

int main(int argc, char** argv, char** envp) {
    fk_puts("\nFKernel Shell v0.1\nType 'exit' to quit.\n\n");

    // Ignore signals in shell process; children will restore to SIG_DFL
    setup_signals((void*)1); // SIG_IGN

    char cwd[MAX_PATH];
    cwd[0] = '/'; cwd[1] = 0;
    sys_getcwd(cwd, sizeof(cwd));
    if (cwd[0] == 0) { cwd[0] = '/'; cwd[1] = 0; }

    char line[MAX_LINE];
    char* args[MAX_ARGS];

    while (1) {
        // Prompt: [cwd] $
        sys_write(1, "[", 1);
        sys_write(1, cwd, fk_strlen(cwd));
        sys_write(1, "] $ ", 4);

        int n = read_line(line, MAX_LINE);
        if (n < 0) break;
        if (n == 0) continue;

        int nargs = parse_args(line, args, MAX_ARGS);
        if (nargs == 0) continue;

        if (fk_strcmp(args[0], "exit") == 0) {
            sys_exit(0);
        }

        if (fk_strcmp(args[0], "cd") == 0) {
            const char* dir = nargs > 1 ? args[1] : "/root";
            if (sys_chdir(dir) < 0) {
                fk_puts("cd: ");
                fk_puts(dir);
                fk_puts(": no such directory\n");
            } else {
                sys_getcwd(cwd, sizeof(cwd));
                if (cwd[0] == 0) { cwd[0] = '/'; cwd[1] = 0; }
            }
            continue;
        }

        if (fk_strcmp(args[0], "pwd") == 0) {
            fk_puts(cwd);
            fk_puts("\n");
            continue;
        }

        int pid = sys_fork();
        if (pid == 0) {
            // Restore default signal handling in child before exec
            setup_signals((void*)0); // SIG_DFL
            exec_command(args, envp);
        }
        if (pid > 0) {
            // Put child in its own process group
            sys_setpgid(pid, pid);
            // Set foreground process group to child's pgid
            int child_pgid = pid;
            sys_ioctl(0, 0x5410 /* TIOCSPGRP */, &child_pgid);
            int status = 0;
            sys_wait4(pid, &status, 0, 0);
            // Restore foreground process group to shell's pgid
            int shell_pgid = sys_getpgid(sys_getpid());
            sys_ioctl(0, 0x5410 /* TIOCSPGRP */, &shell_pgid);
        }
        if (pid < 0) {
            fk_puts("sh: fork failed\n");
        }
    }

    return 0;
}
