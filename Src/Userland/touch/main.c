#include <fk_user.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        print("touch: missing operand\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        // O_CREAT | O_WRONLY | O_TRUNC (using raw numbers if flags are not defined)
        // Usually O_CREAT = 0x40, O_WRONLY = 0x01, O_TRUNC = 0x200 on Linux x86_64
        // Let's check if we have them defined anywhere.
        int fd = sys_open(argv[i], 0x40 | 0x01 | 0x200, 0644);
        if (fd < 0) {
            print("touch: cannot touch ' திரும்பு ");
            print(argv[i]);
            print("\n");
        } else {
            sys_close(fd);
        }
    }

    return 0;
}

