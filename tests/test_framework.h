// Simple test framework for freestanding kernel testing
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <LibC/stddef.h>
#include <LibC/stdint.h>

// Test assertion macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            return message; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            return message; \
        } \
    } while(0)

#define TEST_ASSERT_NE(expected, actual, message) \
    do { \
        if ((expected) == (actual)) { \
            return message; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr, message) \
    TEST_ASSERT((ptr) == nullptr, message)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != nullptr, message)

// Test function pointer type
typedef const char* (*test_func_t)();

// Test case structure
typedef struct {
    const char* name;
    test_func_t func;
} test_case_t;

// Test runner - returns number of failed tests
static int run_tests(const test_case_t* tests, size_t count) {
    int failed = 0;
    
    for (size_t i = 0; i < count; i++) {
        const char* result = tests[i].func();
        if (result == nullptr) {
            // Test passed
        } else {
            failed++;
        }
    }
    
    return failed;
}

#endif // TEST_FRAMEWORK_H