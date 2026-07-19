#include <fk_user.h>

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

static int fk_strlen(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}

static void fk_puts(const char* s) {
    sys_write(1, s, fk_strlen(s));
}

int main(int argc, char** argv, char** envp) {
    struct utsname uts;
    int all = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'a') {
            all = 1;
        }
    }

    if (sys_uname(&uts) < 0) {
        fk_puts("uname: syscall failed\n");
        return 1;
    }

    if (all) {
        fk_puts(uts.sysname);  fk_puts(" ");
        fk_puts(uts.nodename); fk_puts(" ");
        fk_puts(uts.release);  fk_puts(" ");
        fk_puts(uts.version);  fk_puts(" ");
        fk_puts(uts.machine);
        fk_puts("\n");
    } else {
        fk_puts(uts.sysname);
        fk_puts("\n");
    }

    return 0;
}
