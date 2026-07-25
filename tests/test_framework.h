// Simple test framework for freestanding kernel testing
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef FKERNEL_TEST
#define TEST_LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define TEST_LOG(fmt, ...) 
#endif

// Assertion macros - keep it as simple as possible
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  [FAIL] %s\n", message); \
            return message; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("  [FAIL] %s (Expected: %ld, Actual: %ld)\n", message, (long)(expected), (long)(actual)); \
            return message; \
        } \
    } while(0)

#define TEST_ASSERT_STR_EQ(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("  [FAIL] %s (Expected: '%s', Actual: '%s')\n", message, (expected), (actual)); \
            return message; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != NULL, message)

// Test function pointer type
typedef const char* (*test_func_t)();

// Test case structure
typedef struct {
    const char* name;
    test_func_t func;
} test_case_t;

// Test runner helper
static inline int run_tests(const char* suite_name, const test_case_t* tests, size_t count) {
    int failed = 0;
    TEST_LOG(">>> Running %zu tests in suite: %s\n", count, suite_name);
    for (size_t i = 0; i < count; i++) {
        TEST_LOG("  [%zu/%zu] %s... ", i + 1, count, tests[i].name);
        const char* result = tests[i].func();
        if (result == NULL) {
            TEST_LOG("PASS\n");
        } else {
            failed++;
        }
    }
    return failed;
}

#endif // TEST_FRAMEWORK_H
