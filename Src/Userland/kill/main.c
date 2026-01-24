#include <fk_user.h>

long simple_atoi(const char* s) {
    long res = 0;
    int sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res * sign;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print("kill: missing operand\n");
        return 1;
    }

    int sig = 15; // SIGTERM
    int pid_idx = 1;

    if (argv[1][0] == '-') {
        sig = simple_atoi(&argv[1][1]);
        pid_idx = 2;
    }

    if (pid_idx >= argc) {
        print("kill: missing pid\n");
        return 1;
    }

    for (int i = pid_idx; i < argc; i++) {
        int pid = (int)simple_atoi(argv[i]);
        if (sys_kill(pid, sig) < 0) {
            print("kill: failed to kill process\n");
        }
    }

    return 0;
}
