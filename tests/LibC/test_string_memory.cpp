#include "test_framework.h"
#include <LibC/string.h>
#include <LibC/memory.h>

// Test memcpy function
static const char* test_memcpy_basic() {
    char src[10] = "hello";
    char dst[10];
    
    memcpy(dst, src, 6);
    
    TEST_ASSERT_STR_EQ("hello", dst, "memcpy should copy string correctly");
    return nullptr;
}

// Test memcpy with zero size
static const char* test_memcpy_zero_size() {
    char src[10] = "hello";
    char dst[10] = "original";
    
    memcpy(dst, src, 0);
    
    TEST_ASSERT_STR_EQ("original", dst, "memcpy with size 0 should not modify destination");
    return nullptr;
}

// Test memset function
static const char* test_memset_basic() {
    char buffer[10];
    
    memset(buffer, 'A', 5);
    buffer[5] = '\0';
    
    TEST_ASSERT_STR_EQ("AAAAA", buffer, "memset should set bytes correctly");
    return nullptr;
}

// Test strlen function
static const char* test_strlen_basic() {
    const char* str = "hello";
    
    size_t len = strlen(str);
    
    TEST_ASSERT_EQ(5, len, "strlen should return correct length");
    return nullptr;
}

// Test strlen with empty string
static const char* test_strlen_empty() {
    const char* str = "";
    
    size_t len = strlen(str);
    
    TEST_ASSERT_EQ(0, len, "strlen should return 0 for empty string");
    return nullptr;
}

// Test strcpy function
static const char* test_strcpy_basic() {
    const char* src = "hello";
    char dst[10];
    
    strcpy(dst, src);
    
    TEST_ASSERT_STR_EQ("hello", dst, "strcpy should copy string correctly");
    return nullptr;
}

// Test strcmp function
static const char* test_strcmp_equal() {
    const char* str1 = "hello";
    const char* str2 = "hello";
    
    int result = strcmp(str1, str2);
    
    TEST_ASSERT_EQ(0, result, "strcmp should return 0 for equal strings");
    return nullptr;
}

// Test strcmp with different strings
static const char* test_strcmp_different() {
    const char* str1 = "hello";
    const char* str2 = "world";
    
    int result = strcmp(str1, str2);
    
    TEST_ASSERT(result != 0, "strcmp should return non-zero for different strings");
    return nullptr;
}

// Test memset with pattern
static const char* test_memset_pattern() {
    uint8_t buffer[8];
    
    memset(buffer, 0xFF, sizeof(buffer));
    
    for (size_t i = 0; i < sizeof(buffer); i++) {
        TEST_ASSERT_EQ(0xFF, buffer[i], "memset should set all bytes to pattern");
    }
    
    return nullptr;
}

// Test memory operations overlap detection
static const char* test_memmove_basic() {
    char buffer[10] = "hello";
    
    // Move "hello" forward by 2 positions within same buffer
    memmove(buffer + 2, buffer, 5);
    
    TEST_ASSERT_EQ('h', buffer[2], "memmove should handle overlapping correctly");
    TEST_ASSERT_EQ('e', buffer[3], "memmove should handle overlapping correctly");
    TEST_ASSERT_EQ('l', buffer[4], "memmove should handle overlapping correctly");
    TEST_ASSERT_EQ('l', buffer[5], "memmove should handle overlapping correctly");
    TEST_ASSERT_EQ('o', buffer[6], "memmove should handle overlapping correctly");
    
    return nullptr;
}

// LibC test suite
static const test_case_t libc_tests[] = {
    {"test_memcpy_basic", test_memcpy_basic},
    {"test_memcpy_zero_size", test_memcpy_zero_size},
    {"test_memset_basic", test_memset_basic},
    {"test_strlen_basic", test_strlen_basic},
    {"test_strlen_empty", test_strlen_empty},
    {"test_strcpy_basic", test_strcpy_basic},
    {"test_strcmp_equal", test_strcmp_equal},
    {"test_strcmp_different", test_strcmp_different},
    {"test_memset_pattern", test_memset_pattern},
    {"test_memmove_basic", test_memmove_basic},
};

int run_libc_tests() {
    return run_tests(libc_tests, sizeof(libc_tests) / sizeof(libc_tests[0]));
}