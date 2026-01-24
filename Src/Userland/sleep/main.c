#include <fk_user.h>

long simple_atoi(const char* s) {
    long res = 0;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print("sleep: missing operand\n");
        return 1;
    }

    long seconds = simple_atoi(argv[1]);
    struct timespec ts;
    ts.tv_sec = seconds;
    ts.tv_nsec = 0;

    sys_nanosleep(&ts, NULL);

    return 0;
}
