#include <fk_user.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        print("mkdir: missing operand\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (sys_mkdir(argv[i], 0755) < 0) {
            print("mkdir: cannot create directory '");
            print(argv[i]);
            print("': File exists or error\n");
        }
    }

    return 0;
}
