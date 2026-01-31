#include <LibC/string.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

// Simple test assertions
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            return 1; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            return 1; \
        } \
    } while(0)

// Test functions returning 0 for success, 1 for failure
static int test_memcpy() {
    char src[10] = "hello";
    char dst[10];
    
    memcpy(dst, src, 6);
    
    // Simple character-by-character comparison
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQ(dst[i], src[i]);
    }
    
    return 0;
}

static int test_memset() {
    char buffer[10];
    
    memset(buffer, 'A', 5);
    
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQ(buffer[i], 'A');
    }
    
    return 0;
}

static int test_strlen() {
    const char* str = "hello";
    size_t len = strlen(str);
    
    TEST_ASSERT_EQ(len, 5);
    return 0;
}

static int test_strcpy() {
    const char* src = "test";
    char dst[10];
    
    strcpy(dst, src);
    
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQ(dst[i], src[i]);
    }
    
    TEST_ASSERT_EQ(dst[4], '\0');
    return 0;
}

static int test_strcmp() {
    const char* str1 = "hello";
    const char* str2 = "hello";
    const char* str3 = "world";
    
    TEST_ASSERT_EQ(strcmp(str1, str2), 0);
    TEST_ASSERT(strcmp(str1, str3) != 0);
    
    return 0;
}

// Test runner
int main() {
    int failed = 0;
    
    if (test_memcpy() != 0) failed++;
    if (test_memset() != 0) failed++;
    if (test_strlen() != 0) failed++;
    if (test_strcpy() != 0) failed++;
    if (test_strcmp() != 0) failed++;
    
    return failed;
}