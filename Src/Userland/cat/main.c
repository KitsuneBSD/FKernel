#include <fk_user.h>

#define O_RDONLY 0
#define BUF_SIZE 4096

static int fk_strlen(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}

static void fk_puts(const char* s) {
    sys_write(1, s, fk_strlen(s));
}

static void cat_fd(int fd) {
    char buf[BUF_SIZE];
    long n;
    while ((n = sys_read(fd, buf, BUF_SIZE)) > 0) {
        sys_write(1, buf, (size_t)n);
    }
}

static void cat_file(const char* path) {
    int fd = sys_open(path, O_RDONLY, 0);
    if (fd < 0) {
        fk_puts("cat: ");
        fk_puts(path);
        fk_puts(": no such file\n");
        return;
    }
    cat_fd(fd);
    sys_close(fd);
}

int main(int argc, char** argv, char** envp) {
    if (argc < 2) {
        cat_fd(0);
    } else {
        for (int i = 1; i < argc; i++) {
            cat_file(argv[i]);
        }
    }
    return 0;
}
