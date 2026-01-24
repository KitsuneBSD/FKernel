#include <Kernel/Syscall/syscall.h>

extern "C" {

uint64_t sys_getuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    return 0; // Root
}

uint64_t sys_geteuid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    return 0; // Root
}

uint64_t sys_getgid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    return 0; // Root
}

uint64_t sys_getegid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    return 0; // Root
}

uint64_t sys_getpgrp(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    return 0; // Stub
}

}