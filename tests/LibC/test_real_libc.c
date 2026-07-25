#include <tests/test_framework.h>
#include <LibC/string.h>

static const char* test_memcpy_real() {
    char src[10] = "hello";
    char dst[10] = {0};
    
    void* res = memcpy(dst, src, 6);
    
    TEST_ASSERT_EQ((uintptr_t)dst, (uintptr_t)res, "memcpy should return destination pointer");
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQ(src[i], dst[i], "memcpy failed to copy byte correctly");
    }
    
    return NULL;
}

static const char* test_memset_real() {
    char buffer[10] = {0};
    
    void* res = memset(buffer, 'A', 5);
    
    TEST_ASSERT_EQ((uintptr_t)buffer, (uintptr_t)res, "memset should return pointer to buffer");
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQ('A', buffer[i], "memset failed to set byte correctly");
    }
    TEST_ASSERT_EQ(0, buffer[5], "memset set more bytes than requested");
    
    return NULL;
}

static const char* test_strlen_real() {
    TEST_ASSERT_EQ((size_t)5, strlen("hello"), "strlen failed for 'hello'");
    TEST_ASSERT_EQ((size_t)0, strlen(""), "strlen failed for empty string");
    return NULL;
}

static const char* test_strcmp_real() {
    TEST_ASSERT_EQ(0, strcmp("abc", "abc"), "strcmp failed for equal strings");
    TEST_ASSERT(strcmp("abc", "abd") < 0, "strcmp failed for 'abc' < 'abd'");
    TEST_ASSERT(strcmp("abd", "abc") > 0, "strcmp failed for 'abd' > 'abc'");
    return NULL;
}

static const char* test_strcpy_real() {
    char dst[10];
    char* res = strcpy(dst, "world");
    
    TEST_ASSERT_EQ((uintptr_t)dst, (uintptr_t)res, "strcpy should return destination pointer");
    TEST_ASSERT_EQ(0, strcmp(dst, "world"), "strcpy failed to copy string");
    return NULL;
}

int main() {
    test_case_t tests[] = {
        {"memcpy", test_memcpy_real},
        {"memset", test_memset_real},
        {"strlen", test_strlen_real},
        {"strcmp", test_strcmp_real},
        {"strcpy", test_strcpy_real}
    };
    
    int failed = run_tests(tests, sizeof(tests) / sizeof(tests[0]));
    return failed;
}
