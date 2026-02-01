#include <tests/test_framework.h>

// Mock implementation of kernel assert failure for tests
// test_framework.h is automatically included via xmake -include

void __kernel_assert_fail(const char *expr, const char *file, int line, const char *func) {
    // We use ::fprintf to access host version directly if needed, 
    // but here we just want to report and exit.
    TEST_LOG("ASSERTION FAILED: %s\n", expr);
    TEST_LOG("at %s:%d in %s\n", file, line, func);
    exit(1);
}
