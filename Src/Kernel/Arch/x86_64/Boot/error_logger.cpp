#include <LibFK/Algorithms/log.h>

extern "C" void kernel_panic_log(const char* msg) {
    fk::algorithms::kerror("PANIC", "%s", msg);
}
