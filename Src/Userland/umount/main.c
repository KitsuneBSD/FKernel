#include <fk_user.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        print("umount: missing operand\n");
        return 1;
    }

    if (sys_umount(argv[1], 0) < 0) {
        print("umount: failed to unmount\n");
        return 1;
    }

    return 0;
}

