#include <fk_user.h>

static int fk_strlen(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void fk_puts(const char* s) {
    sys_write(1, s, fk_strlen(s));
}

static void setup_mounts(void) {
    sys_mount("tmpfs", "/tmp",     "tmpfs", 0, 0);
    sys_mount("tmpfs", "/var/run", "tmpfs", 0, 0);
    sys_mount("tmpfs", "/var/tmp", "tmpfs", 0, 0);
}

int main(int argc, char** argv, char** envp) {
    (void)argc; (void)argv; (void)envp;
    fk_puts("\nFKernel init starting...\n");

    setup_mounts();

    while (1) {
        int pid = sys_fork();
        if (pid == 0) {
            sys_setsid();

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
