#include <fk_user.h>

int main(int argc, char** argv, char** envp) {
    // ANSI: clear screen + move cursor to top-left
    sys_write(1, "\033[2J\033[H", 7);
    return 0;
}
