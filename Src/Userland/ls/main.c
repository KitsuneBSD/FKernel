#include <fk_user.h>

int main(int argc, char** argv) {
    char* path = ".";
    if (argc > 1) {
        path = argv[1];
    }

    int fd = sys_open(path, 0x10000); // O_DIRECTORY
    if (fd < 0) {
        fd = sys_open(path, 0);
    }
    
    if (fd < 0) {
        print("ls: cannot access '");
        print(path);
        print("': No such file or directory\n");
        return 1;
    }

    char buffer[4096];
    int64_t n;
    // Use getdents64 which now returns text listing
    while ((n = sys_getdents64(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        print(buffer);
    }

    if (n < 0) {
        print("ls: error reading directory\n");
    }

    sys_close(fd);
    return 0;
}