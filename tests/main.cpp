#include <tests/test_framework.h>

// Forward declarations of test runners
int run_libc_string_tests();
int run_libfk_container_tests();
int run_libfk_container_advanced_tests();
int run_libfk_smart_pointer_tests();
int run_libfk_text_tests();
int run_libfk_multi_container_tests();
int run_libfk_tuple_tests();

int main() {
    int failed = 0;

    failed += run_libc_string_tests();
    failed += run_libfk_container_tests();
    failed += run_libfk_container_advanced_tests();
    failed += run_libfk_smart_pointer_tests();
    failed += run_libfk_text_tests();
    failed += run_libfk_multi_container_tests();
    failed += run_libfk_tuple_tests();
    
    if (failed == 0) {
        TEST_LOG("\n>>> SUMMARY: ALL TEST SUITES PASSED!\n");
    } else {
        TEST_LOG("\n>>> SUMMARY: %d TOTAL TESTS FAILED!\n", failed);
    }
    
    return failed != 0;
}
