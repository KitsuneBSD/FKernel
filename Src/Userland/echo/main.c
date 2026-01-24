#include <fk_user.h>

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        print(argv[i]);
        if (i < argc - 1) {
            print(" ");
        }
    }
    print("\n");
    return 0;
}
