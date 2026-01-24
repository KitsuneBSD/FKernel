#include <fk_user.h>

int main() {
    char buffer[1024];
    if (sys_getcwd(buffer, sizeof(buffer)) < 0) {
        print("pwd: error getting current directory\n");
        return 1;
    }
    print(buffer);
    print("\n");
    return 0;
}
