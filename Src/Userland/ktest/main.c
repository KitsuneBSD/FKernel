#include <fk_user.h>

/* ── helpers ──────────────────────────────────────────────── */

void *memset(void *s, int c, unsigned long n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

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

static int fk_strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
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
    int pid = sys_fork();
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

/* ── Regression tests (Phase 17g) ─────────────────────────── */

/* TmpFs write + read back: verifies write_to_cluster_chain path */
static void test_tmpfs_write_read(void) {
    const char *path = "/tmp/ktest_wr.dat";
    const char *data = "Hello, FKernel!";
    int datalen = fk_strlen(data);

    /* create + write */
    int fd = sys_open(path, 0x241 /* O_CREAT|O_WRONLY|O_TRUNC, mode 0644 */);
    if (fd < 0) { fail("tmpfs_write_read", "open for write failed"); return; }
    long written = sys_write(fd, data, datalen);
    sys_close(fd);
    if (written != datalen) { fail("tmpfs_write_read", "write returned wrong count"); return; }

    /* read back */
    fd = sys_open(path, 0 /* O_RDONLY */);
    if (fd < 0) { fail("tmpfs_write_read", "open for read failed"); return; }
    char buf[64] = {};
    long nread = sys_read(fd, buf, sizeof(buf));
    sys_close(fd);
    if (nread != datalen) { fail("tmpfs_write_read", "read returned wrong count"); return; }
    if (fk_strcmp(buf, data) != 0) { fail("tmpfs_write_read", "read data mismatch"); return; }
    pass("tmpfs_write_read");
}

/* Multi-byte write: write more than one cluster's worth */
static void test_tmpfs_large_write(void) {
    const char *path = "/tmp/ktest_large.dat";
    /* 1024 bytes = 2 clusters */
    char wbuf[1024];
    for (int i = 0; i < 1024; i++) wbuf[i] = (char)(i & 0xFF);

    int fd = sys_open(path, 0x241);
    if (fd < 0) { fail("tmpfs_large_write", "open failed"); return; }
    long written = sys_write(fd, wbuf, 1024);
    sys_close(fd);
    if (written != 1024) { fail("tmpfs_large_write", "write count mismatch"); return; }

    fd = sys_open(path, 0);
    if (fd < 0) { fail("tmpfs_large_write", "open for read failed"); return; }
    char rbuf[1024] = {};
    long nread = sys_read(fd, rbuf, 1024);
    sys_close(fd);
    if (nread != 1024) { fail("tmpfs_large_write", "read count mismatch"); return; }

    int ok = 1;
    for (int i = 0; i < 1024; i++) {
        if (rbuf[i] != (char)(i & 0xFF)) { ok = 0; break; }
    }
    if (ok) pass("tmpfs_large_write");
    else    fail("tmpfs_large_write", "data corruption");
}

/* dup2 redirects stdout to file */
static void test_dup2(void) {
    const char *path = "/tmp/ktest_dup2.txt";
    const char *msg = "dup2_ok";

    int fd = sys_open(path, 0x241);
    if (fd < 0) { fail("dup2", "open failed"); return; }

    int old_stdout = sys_dup2(1, 100);  /* save stdout as fd 100 */
    sys_dup2(fd, 1);                     /* redirect stdout to file */
    sys_close(fd);

    sys_write(1, msg, fk_strlen(msg));   /* write to redirected stdout */

    sys_dup2(old_stdout, 1);             /* restore stdout */
    sys_close(old_stdout);

    /* read the file back */
    fd = sys_open(path, 0);
    if (fd < 0) { fail("dup2", "open for read failed"); return; }
    char buf[32] = {};
    long n = sys_read(fd, buf, sizeof(buf));
    sys_close(fd);
    sys_close(100);

    if (n != fk_strlen(msg)) { fail("dup2", "read count mismatch"); return; }
    if (fk_strcmp(buf, msg) != 0) { fail("dup2", "data mismatch"); return; }
    pass("dup2");
}

/* fstat returns valid fields */
static void test_fstat(void) {
    struct { unsigned long fields[18]; } buf;
    int fd = sys_open("/bin/sh", 0);
    if (fd < 0) { fail("fstat", "open failed"); return; }
    int r = sys_fstat(fd, &buf);
    sys_close(fd);
    if (r == 0) pass("fstat");
    else        fail("fstat", "fstat failed");
}

/* getuid/geteuid/getgid/getegid return non-negative */
static void test_ids(void) {
    int uid  = sys_getuid();
    int euid = sys_geteuid();
    int gid  = sys_getgid();
    int egid = sys_getegid();
    if (uid >= 0 && euid >= 0 && gid >= 0 && egid >= 0)
        pass("ids");
    else
        fail("ids", "one or more id calls returned negative");
}

/* mkdir + stat directory */
static void test_mkdir_stat(void) {
    const char *dir = "/tmp/ktest_dir";
    sys_mkdir(dir, 0755);
    struct { unsigned long fields[18]; } buf;
    int r = sys_stat(dir, &buf);
    /* cleanup */
    /* Note: no rmdir syscall exposed yet */
    if (r == 0) pass("mkdir_stat");
    else        fail("mkdir_stat", "stat on created dir failed");
}

/* getdents64 on /tmp */
static void test_getdents(void) {
    int fd = sys_open("/tmp", 0 /* O_RDONLY */);
    if (fd < 0) { fail("getdents", "open /tmp failed"); return; }
    char buf[1024];
    long n = sys_getdents64(fd, buf, sizeof(buf));
    sys_close(fd);
    if (n > 0) pass("getdents");
    else       fail("getdents", "getdents returned <= 0");
}

/* ── Signal Regression Tests ─────────────────────────────── */

/* Verify fork() copies pgid from parent to child */
static void test_fork_pgid(void) {
    int parent_pgid = sys_getpgid(sys_getpid());
    if (parent_pgid <= 0) { fail("fork_pgid", "getpgid failed"); return; }

    int pid = sys_fork();
    if (pid < 0) { fail("fork_pgid", "fork failed"); return; }
    if (pid == 0) {
        int child_pgid = sys_getpgid(sys_getpid());
        sys_exit(child_pgid == parent_pgid ? 0 : 1);
    }
    int status = 0;
    sys_wait4(pid, &status, 0, 0);
    int code = (status >> 8) & 0xff;
    if (code == 0) pass("fork_pgid");
    else           fail("fork_pgid", "child pgid != parent pgid");
}

/* Verify setpgid() changes pgid */
static void test_setpgid(void) {
    int orig_pgid = sys_getpgid(sys_getpid());
    int new_pgid = sys_getpid();

    int r = sys_setpgid(sys_getpid(), new_pgid);
    if (r < 0) { fail("setpgid", "setpgid syscall failed"); return; }

    int now = sys_getpgid(sys_getpid());
    /* restore */
    sys_setpgid(sys_getpid(), orig_pgid);

    if (now == new_pgid) pass("setpgid");
    else                 fail("setpgid", "pgid did not change");
}

/* Verify fork() child inherits sid */
static void test_fork_sid(void) {
    int pid = sys_fork();
    if (pid < 0) { fail("fork_sid", "fork failed"); return; }
    if (pid == 0) {
        /* setsid sets sid=pgid=pid; check child of setsid inherits it */
        sys_setsid();
        int sid = sys_getpgid(sys_getpid());
        sys_exit(sid == sys_getpid() ? 0 : 1);
    }
    int status = 0;
    sys_wait4(pid, &status, 0, 0);
    int code = (status >> 8) & 0xff;
    if (code == 0) pass("fork_sid");
    else           fail("fork_sid", "setsid did not set sid to own pid");
}

/* Send SIGUSR1 to child via kill(); child should terminate with signal */
static void test_signal_kill(void) {
    int pid = sys_fork();
    if (pid < 0) { fail("signal_kill", "fork failed"); return; }
    if (pid == 0) {
        /* Sleep until killed */
        struct timespec ts = { 30, 0 };
        sys_nanosleep(&ts, 0);
        sys_exit(0); /* should not reach here */
    }
    /* Let child enter sleep */
    struct timespec ts = { 0, 50000000 }; /* 50ms */
    sys_nanosleep(&ts, 0);

    sys_kill(pid, 10 /* SIGUSR1 */);

    int status = 0;
    sys_wait4(pid, &status, 0, 0);
    /* killed by signal: low byte == signal number */
    if ((status & 0x7f) == 10) pass("signal_kill");
    else                       fail("signal_kill", "child not killed by SIGUSR1");
}

/* Send signal to process group; child should die, parent survives */
static void test_signal_group(void) {
    /* Parent must ignore SIGUSR1 so it survives the group signal */
    struct sigaction ign;
    ign.sa_handler = (void (*)(int))1; /* SIG_IGN */
    ign.sa_flags = 0;
    ign.sa_restorer = 0;
    ign.sa_mask = 0;
    sys_sigaction(10 /* SIGUSR1 */, &ign, 0);

    int parent_pgid = sys_getpgid(sys_getpid());

    int pid = sys_fork();
    if (pid < 0) { fail("signal_group", "fork failed"); return; }
    if (pid == 0) {
        /* Child: same pgid as parent (inherited). Restore SIG_DFL so it dies. */
        struct sigaction dfl;
        dfl.sa_handler = (void (*)(int))0; /* SIG_DFL */
        dfl.sa_flags = 0;
        dfl.sa_restorer = 0;
        dfl.sa_mask = 0;
        sys_sigaction(10 /* SIGUSR1 */, &dfl, 0);

        struct timespec ts = { 30, 0 };
        sys_nanosleep(&ts, 0);
        sys_exit(0);
    }

    struct timespec ts = { 0, 50000000 };
    sys_nanosleep(&ts, 0);

    /* Send SIGUSR1 to the entire process group (negative pgid) */
    sys_kill(-parent_pgid, 10 /* SIGUSR1 */);

    int status = 0;
    sys_wait4(pid, &status, 0, 0);
    /* Child should be killed by SIGUSR1 */
    if ((status & 0x7f) == 10) pass("signal_group");
    else                       fail("signal_group", "child not killed by group signal");
}

/* Install a signal handler, send signal, verify handler ran and sigreturn works */
static volatile int g_handler_ran;

static void test_handler(int sig) {
    (void)sig;
    g_handler_ran = 1;
}

static void test_signal_handler(void) {
    g_handler_ran = 0;

    int pid = sys_fork();
    if (pid < 0) { fail("signal_handler", "fork failed"); return; }
    if (pid == 0) {
        struct sigaction act;
        act.sa_handler = test_handler;
        act.sa_flags = 0;
        act.sa_restorer = 0; /* use kernel builtin restorer */
        act.sa_mask = 0;
        sys_sigaction(10 /* SIGUSR1 */, &act, 0);

        /* Wait for signal (up to 5 seconds) */
        struct timespec ts = { 5, 0 };
        sys_nanosleep(&ts, 0);

        sys_exit(g_handler_ran ? 0 : 2);
    }

    struct timespec ts = { 0, 100000000 }; /* 100ms */
    sys_nanosleep(&ts, 0);

    sys_kill(pid, 10 /* SIGUSR1 */);

    int status = 0;
    sys_wait4(pid, &status, 0, 0);
    int code = (status >> 8) & 0xff;
    if (code == 0) pass("signal_handler");
    else           fail("signal_handler", "handler did not run or sigreturn failed");
}

/* SIGCHLD: parent wait4() after child exits reaps zombie correctly */
static void test_sigchld_reap(void) {
    int pid = sys_fork();
    if (pid < 0) { fail("sigchld_reap", "fork failed"); return; }
    if (pid == 0) {
        sys_exit(99);
    }
    /* Wait without specifying pid - any child */
    int status = 0;
    int waited = sys_wait4(-1, &status, 0, 0);
    int code = (status >> 8) & 0xff;
    if (waited == pid && code == 99) pass("sigchld_reap");
    else                             fail("sigchld_reap", "wait4(-1) failed or wrong exit code");
}

/* Foreground process group: setpgid + TIOCSPGRP roundtrip */
static void test_foreground_pgrp(void) {
    int orig = sys_getpgid(sys_getpid());

    int new_pgid = sys_getpid();
    sys_setpgid(sys_getpid(), new_pgid);

    int fg = -1;
    sys_ioctl(0, 0x5410 /* TIOCSPGRP */, &new_pgid);
    sys_ioctl(0, 0x540F /* TIOCGPGRP */, &fg);

    /* restore */
    sys_ioctl(0, 0x5410 /* TIOCSPGRP */, &orig);
    sys_setpgid(sys_getpid(), orig);

    if (fg == new_pgid) pass("foreground_pgrp");
    else                fail("foreground_pgrp", "TIOCGPGRP did not match TIOCSPGRP");
}

/* ── main ─────────────────────────────────────────────────── */

int main(void) {
    g_serial_fd = sys_open("/dev/ttyS0", 1 /* O_WRONLY */);

    puts_("\n=== FKernel Test Suite ===\n\n");

    puts_("[Process]\n");
    test_getpid();
    test_getppid();
    test_getuid();
    test_ids();

    puts_("\n[IO]\n");
    test_write();
    test_open_close();
    test_stat();
    test_fstat();
    test_getcwd();
    test_chdir();
    test_getdents();

    puts_("\n[Scheduler]\n");
    test_fork_wait();
    test_exit_status();
    test_multiple_children();

    puts_("\n[Exec]\n");
    test_fork_exec();
    test_vfork_exec();

    puts_("\n[VFS Regression]\n");
    test_tmpfs_write_read();
    test_tmpfs_large_write();
    test_dup2();
    test_mkdir_stat();

    puts_("\n[Signal Regression]\n");
    test_fork_pgid();
    test_setpgid();
    test_fork_sid();
    test_signal_kill();
    test_signal_group();
    test_signal_handler();
    test_sigchld_reap();
    test_foreground_pgrp();

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
