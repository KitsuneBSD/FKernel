#include <fk_user.h>

int main(int argc, char** argv) {
    if (argc < 4) {
        print("Usage: mount <device> <target> <type>\n");
        return 1;
    }

    const char* device = argv[1];
    const char* target = argv[2];
    const char* type = argv[3];

    if (sys_mount(device, target, type, 0, NULL) < 0) {
        print("mount: failed to mount\n");
        return 1;
    }

    return 0;
}
