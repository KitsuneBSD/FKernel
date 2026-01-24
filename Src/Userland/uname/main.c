#include <fk_user.h>

int main() {
    struct {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
        char domainname[65];
    } uts;

    if (sys_uname(&uts) == 0) {
        print(uts.sysname);
        print(" ");
        print(uts.release);
        print(" ");
        print(uts.machine);
        print("\n");
    } else {
        print("uname: syscall failed\n");
        return 1;
    }
    return 0;
}

