#include <fk_user.h>

static int fk_strlen(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void fk_puts(const char* s) {
    sys_write(1, s, fk_strlen(s));
}

static void ensure_dir(const char* path) {
    sys_mkdir(path, 0755);
}

static void try_mount(const char* src, const char* target, const char* fstype) {
    ensure_dir(target);
    int r = sys_mount(src, target, fstype, 0, 0);
    if (r < 0) {
        fk_puts("init: mount failed: ");
        fk_puts(target);
        fk_puts("\n");
    }
}

static void setup_mounts(void) {
    try_mount("tmpfs", "/tmp",     "tmpfs");
    try_mount("tmpfs", "/var/run", "tmpfs");
    try_mount("tmpfs", "/var/tmp", "tmpfs");
}

static void run_ktest(void) {
    int pid = sys_fork();
    if (pid == 0) {
        char* argv[] = { "/bin/ktest", 0 };
        char* envp[] = { "PATH=/bin:/sbin", 0 };
        sys_execve("/bin/ktest", argv, envp);
        sys_exit(127);
    }
    if (pid > 0) {
        int status = 0;
        sys_wait4(pid, &status, 0, 0);
    }
}

int main(int argc, char** argv, char** envp) {
    (void)argc; (void)argv; (void)envp;
    fk_puts("\nFKernel init starting...\n");

    setup_mounts();
    run_ktest();

    while (1) {
        int pid = sys_fork();
        if (pid == 0) {
            sys_setsid();
            // Set this process group as the foreground process group of the terminal
            sys_ioctl(0, 0x540E /* TIOCSCTTY */, 0);

            char* sh_argv[] = { "/bin/sh", 0 };
            char* sh_envp[] = {
                "PATH=/bin:/sbin:/usr/bin:/usr/sbin",
                "HOME=/root",
                "TERM=vt100",
                "SHELL=/bin/sh",
                "USER=root",
                "LOGNAME=root",
                0
            };
            sys_execve("/bin/sh", sh_argv, sh_envp);
            fk_puts("init: failed to exec /bin/sh\n");
            sys_exit(1);
        }
        if (pid > 0) {
            int status = 0;
            sys_wait4(pid, &status, 0, 0);
            fk_puts("init: shell exited, restarting...\n");
        }
        struct timespec ts = { 1, 0 };
        sys_nanosleep(&ts, 0);
    }

    return 0;
}
