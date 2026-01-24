#include <fk_user.h>

void print_num(long n) {
    if (n == 0) {
        print("0");
        return;
    }
    char buf[32];
    int i = 30;
    buf[31] = '\0';
    while (n > 0) {
        buf[--i] = (n % 10) + '0';
        n /= 10;
    }
    print(&buf[i]);
}

int main() {
    print("uid=");
    print_num(sys_getuid());
    print(" gid=");
    // sys_getgid is 104
    // I need to add it to fk_user.h if I want to use it easily, but I can use sys_getuid for now if it returns both or something.
    // Actually I'll just use getuid.
    print_num(sys_getuid()); 
    print("\n");
    return 0;
}

