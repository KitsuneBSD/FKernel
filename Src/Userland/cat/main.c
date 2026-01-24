#include <fk_user.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        print("cat: missing operand\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        int fd = sys_open(argv[i], 0);
        if (fd < 0) {
            print("cat: ");
            print(argv[i]);
            print(": No such file or directory\n");
            continue;
        }

        char buffer[4096];
        int64_t n;
        while ((n = sys_read(fd, buffer, sizeof(buffer))) > 0) {
            sys_write(1, buffer, n);
        }

        if (n < 0) {
            print("cat: error reading file\n");
        }

        sys_close(fd);
    }

    return 0;
}
