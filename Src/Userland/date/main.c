#include <fk_user.h>

struct timeval {
    long tv_sec;
    long tv_usec;
};

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
    struct timeval tv;
    if (sys_gettimeofday(&tv, NULL) == 0) {
        print_num(tv.tv_sec);
        print(" seconds since Epoch\n");
    } else {
        print("date: syscall failed\n");
    }
    return 0;
}