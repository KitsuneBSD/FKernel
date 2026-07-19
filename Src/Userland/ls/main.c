#include <fk_user.h>

#define O_RDONLY  0
#define O_DIRECTORY 0200000

#define BUF_SIZE 4096

static int fk_strlen(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}

static void fk_puts(const char* s) {
    sys_write(1, s, fk_strlen(s));
}

static void list_dir(const char* path) {
    int fd = sys_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) {
        fd = sys_open(path, O_RDONLY, 0);
        if (fd < 0) {
            fk_puts("ls: cannot open: ");
            fk_puts(path);
            fk_puts("\n");
            return;
        }
    }

    char buf[BUF_SIZE];
    long n;
    while ((n = sys_getdents64(fd, buf, BUF_SIZE)) > 0) {
        long off = 0;
        while (off < n) {
            struct linux_dirent64* d = (struct linux_dirent64*)(buf + off);
            if (d->d_name[0] != '.') {
                fk_puts(d->d_name);
                fk_puts("\n");
            }
            off += d->d_reclen;
        }
    }
    sys_close(fd);
}

int main(int argc, char** argv, char** envp) {
    if (argc < 2) {
        list_dir(".");
    } else {
        for (int i = 1; i < argc; i++) {
            if (argc > 2) {
                fk_puts(argv[i]);
                fk_puts(":\n");
            }
            list_dir(argv[i]);
        }
    }
    return 0;
}
