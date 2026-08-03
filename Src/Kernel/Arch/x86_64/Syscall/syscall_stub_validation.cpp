#include <LibFK/Algorithms/Logging/log.h>

extern "C" void syscall_validation_log(uint64_t syscall_num) {
    fk::algorithms::kerror("SYSCALL", "Invalid syscall number: %lu (Max: 512)", syscall_num);
}

extern "C" void user_mode_validation_log(const char* reason, uint64_t val) {
    fk::algorithms::kerror("USERMODE", "Validation failed: %s (Value: 0x%lx)", reason, val);
}
