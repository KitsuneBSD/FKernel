#include <fk_user.h>

/* ── helpers ──────────────────────────────────────────────── */

static int g_pass = 0;
static int g_fail = 0;
static int g_serial_fd = -1;

static int fk_strlen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static void puts_(const char *s) {
    int len = fk_strlen(s);
    sys_write(1, s, len);
    if (g_serial_fd >= 0) sys_write(g_serial_fd, s, len);
}

static void print_int(int n) {
    if (n < 0) { puts_("-"); n = -n; }
    char buf[12]; int i = 11;
    buf[i] = 0;
    if (n == 0) { buf[--i] = '0'; }
    else { while (n) { buf[--i] = '0' + (n % 10); n /= 10; } }
    puts_(buf + i);
}

static void pass(const char *name) {
    puts_("  [ PASS ] "); puts_(name); puts_("\n");
    g_pass++;
}

static void fail(const char *name, const char *reason) {
    puts_("  [ FAIL ] "); puts_(name); puts_(": "); puts_(reason); puts_("\n");
    g_fail++;
}

/* ── individual tests ─────────────────────────────────────── */

static void test_getpid(void) {
    int pid = sys_getpid();
    if (pid > 0) pass("getpid");
    else         fail("getpid", "returned non-positive PID");
}

static void test_getppid(void) {
    int ppid = sys_getppid();
    if (ppid > 0) pass("getppid");
    else          fail("getppid", "returned non-positive PPID");
}

static void test_write(void) {
    /* write to /dev/null (fd 2 is stderr, just write there for a side-effect test) */
    long n = sys_write(1, ".", 1);
    if (n == 1) { puts_(" "); pass("write"); }
    else        fail("write", "sys_write returned wrong count");
}

static void test_fork_wait(void) {
    int pid = sys_fork();
    if (pid < 0) { fail("fork_wait", "fork failed"); return; }
    if (pid == 0) {
        sys_exit(42);
    }
    int status = 0;
    int waited = sys_wait4(pid, &status, 0, 0);
    if (waited != pid) { fail("fork_wait", "wait4 returned wrong pid"); return; }
    int code = (status >> 8) & 0xff;
    if (code == 42) pass("fork_wait");
    else            fail("fork_wait", "exit status mismatch");
}

static void test_exit_status(void) {
    int pid = sys_fork();
    if (pid < 0) { fail("exit_status", "fork failed"); return; }
    if (pid == 0) {
        sys_exit(7);
    }
    int status = 0;
    sys_wait4(pid, &status, 0, 0);
    int code = (status >> 8) & 0xff;
    if (code == 7) pass("exit_status");
    else           fail("exit_status", "wrong exit code");
}

static void test_multiple_children(void) {
    int pids[4];
    for (int i = 0; i < 4; i++) {
        int pid = sys_fork();
        if (pid < 0) { fail("multiple_children", "fork failed"); return; }
        if (pid == 0) sys_exit(i);
        pids[i] = pid;
    }
    int ok = 1;
    for (int i = 0; i < 4; i++) {
        int status = 0;
        int waited = sys_wait4(pids[i], &status, 0, 0);
        int code = (status >> 8) & 0xff;
        if (waited != pids[i] || code != i) ok = 0;
    }
    if (ok) pass("multiple_children");
    else    fail("multiple_children", "pid or exit-code mismatch");
}

static void test_fork_exec(void) {
    int pid = sys_fork();
    if (pid < 0) { fail("fork_exec", "fork failed"); return; }
    if (pid == 0) {
        char *argv[] = { "/bin/true", 0 };
        char *envp[] = { 0 };
        sys_execve("/bin/true", argv, envp);
        sys_exit(1);
    }
    int status = 0;
    sys_wait4(pid, &status, 0, 0);
    int code = (status >> 8) & 0xff;
    if (code == 0) pass("fork_exec");
    else           fail("fork_exec", "/bin/true exited non-zero");
}

static void test_vfork_exec(void) {
    int pid = sys_fork();   /* use fork as proxy: vfork is tested via init */
    if (pid < 0) { fail("vfork_exec", "fork failed"); return; }
    if (pid == 0) {
        char *argv[] = { "/bin/false", 0 };
        char *envp[] = { 0 };
        sys_execve("/bin/false", argv, envp);
        sys_exit(0);
    }
    int status = 0;
    sys_wait4(pid, &status, 0, 0);
    int code = (status >> 8) & 0xff;
    if (code == 1) pass("vfork_exec");
    else           fail("vfork_exec", "/bin/false should exit 1");
}

static void test_stat(void) {
    /* stat /bin/sh — must exist */
    struct { unsigned long fields[18]; } buf;
    int r = sys_stat("/bin/sh", &buf);
    if (r == 0) pass("stat");
    else        fail("stat", "/bin/sh stat failed");
}

static void test_open_close(void) {
    int fd = sys_open("/bin/sh", 0 /* O_RDONLY */);
    if (fd < 0) { fail("open_close", "open /bin/sh failed"); return; }
    int r = sys_close(fd);
    if (r == 0) pass("open_close");
    else        fail("open_close", "close returned error");
}

static void test_getcwd(void) {
    char buf[256];
    int r = sys_getcwd(buf, sizeof(buf));
    if (r >= 0 && buf[0] == '/') pass("getcwd");
    else                         fail("getcwd", "invalid cwd");
}

static void test_chdir(void) {
    char before[256], after[256];
    sys_getcwd(before, sizeof(before));
    int r = sys_chdir("/tmp");
    if (r < 0) { fail("chdir", "chdir /tmp failed"); return; }
    sys_getcwd(after, sizeof(after));
    sys_chdir(before);
    /* after should start with /tmp */
    if (after[0] == '/' && after[1] == 't' && after[2] == 'm' && after[3] == 'p')
        pass("chdir");
    else
        fail("chdir", "cwd did not change to /tmp");
}

static void test_getuid(void) {
    int uid = sys_getuid();
    if (uid >= 0) pass("getuid");
    else          fail("getuid", "returned negative uid");
}

/* ── main ─────────────────────────────────────────────────── */

int main(void) {
    g_serial_fd = sys_open("/dev/ttyS0", 1 /* O_WRONLY */);

    puts_("\n=== FKernel Test Suite ===\n\n");

    puts_("[Process]\n");
    test_getpid();
    test_getppid();
    test_getuid();

    puts_("\n[IO]\n");
    test_write();
    test_open_close();
    test_stat();
    test_getcwd();
    test_chdir();

    puts_("\n[Scheduler]\n");
    test_fork_wait();
    test_exit_status();
    test_multiple_children();

    puts_("\n[Exec]\n");
    test_fork_exec();
    test_vfork_exec();

    puts_("\n");
    puts_("=========================\n");
    puts_("Results: ");
    print_int(g_pass); puts_(" passed, ");
    print_int(g_fail); puts_(" failed\n");
    puts_("=========================\n\n");

    if (g_serial_fd >= 0) sys_close(g_serial_fd);
    sys_exit(g_fail > 0 ? 1 : 0);
    return 0;
}
