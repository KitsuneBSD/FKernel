#include "test_framework.h"
#include <LibC/memory.h>
#include <LibC/string.h>

// Test memcpy function
static const char* test_memcpy_basic() {
  char src[10] = "hello";
  char dst[10];

  memcpy(dst, src, 6);

  TEST_ASSERT_STR_EQ("hello", dst, "memcpy should copy string correctly");
  return nullptr;
}

static const char* test_memcpy_zero_size() {
  char src[10] = "hello";
  char dst[10] = "original";

  memcpy(dst, src, 0);

  TEST_ASSERT_STR_EQ("original", dst, "memcpy with size 0 should not modify destination");
  return nullptr;
}

static const char* test_memcpy_overlap() {
  char buffer[20] = "1234567890";

  memmove(buffer + 2, buffer, 5);

  TEST_ASSERT_STR_EQ("121234567890", buffer, "memmove should handle overlap correctly");
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

static const char* test_memset_all() {
  uint8_t buffer[8];

  memset(buffer, 0xFF, sizeof(buffer));

  for (size_t i = 0; i < sizeof(buffer); i++) {
    TEST_ASSERT_EQ(0xFF, buffer[i], "memset should set all bytes to pattern");
  }

  return nullptr;
}

// Test memcmp function
static const char* test_memcmp_equal() {
  char buf1[] = "hello";
  char buf2[] = "hello";

  int result = memcmp(buf1, buf2, 5);

  TEST_ASSERT_EQ(0, result, "memcmp should return 0 for equal buffers");
  return nullptr;
}

static const char* test_memcmp_different() {
  char buf1[] = "hello";
  char buf2[] = "world";

  int result = memcmp(buf1, buf2, 5);

  TEST_ASSERT(result != 0, "memcmp should return non-zero for different buffers");
  return nullptr;
}

// Test strlen function
static const char* test_strlen_basic() {
  const char* str = "hello";

  size_t len = strlen(str);

  TEST_ASSERT_EQ(5, len, "strlen should return correct length");
  return nullptr;
}

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

static const char* test_strcmp_less() {
  const char* str1 = "apple";
  const char* str2 = "banana";

  int result = strcmp(str1, str2);

  TEST_ASSERT(result < 0, "strcmp should return negative when first < second");
  return nullptr;
}

static const char* test_strcmp_greater() {
  const char* str1 = "zebra";
  const char* str2 = "apple";

  int result = strcmp(str1, str2);

  TEST_ASSERT(result > 0, "strcmp should return positive when first > second");
  return nullptr;
}

// Test strncmp function
static const char* test strncmp_basic() {
  const char* str1 = "hello world";
  const char* str2 = "hello there";

  int result = strncmp(str1, str2, 5);

  TEST_ASSERT_EQ(0, result, "strncmp should return 0 for equal prefix");
  return nullptr;
}

// Test strchr function
static const char* test_strchr_found() {
  const char* str = "hello world";
  char* result = strchr(str, 'w');

  TEST_ASSERT_NOT_NULL(result, "strchr should find character");
  TEST_ASSERT_STR_EQ("world", result, "strchr should return pointer to found character");
  return nullptr;
}

static const char* test_strchr_not_found() {
  const char* str = "hello world";
  char* result = strchr(str, 'z');

  TEST_ASSERT_NULL(result, "strchr should return nullptr when character not found");
  return nullptr;
}

// Test strrchr function
static const char* test_strrchr_found() {
  const char* str = "hello hello";
  char* result = strrchr(str, 'l');

  TEST_ASSERT_NOT_NULL(result, "strrchr should find last occurrence");
  TEST_ASSERT_STR_EQ("lo", result, "strrchr should return pointer to last occurrence");
  return nullptr;
}

// Test strcat function
static const char* test_strcat_basic() {
  char dst[20] = "hello ";
  const char* src = "world";

  strcat(dst, src);

  TEST_ASSERT_STR_EQ("hello world", dst, "strcat should concatenate strings");
  return nullptr;
}

// Test strncpy function
static const char* test_strncpy_basic() {
  const char* src = "hello world";
  char dst[10];

  strncpy(dst, src, 5);
  dst[5] = '\0';

  TEST_ASSERT_STR_EQ("hello", dst, "strncpy should copy specified characters");
  return nullptr;
}

// Test atoi function
static const char* test_atoi_positive() {
  const char* str = "123";
  int result = atoi(str);

  TEST_ASSERT_EQ(123, result, "atoi should convert positive number");
  return nullptr;
}

static const char* test_atoi_negative() {
  const char* str = "-456";
  int result = atoi(str);

  TEST_ASSERT_EQ(-456, result, "atoi should convert negative number");
  return nullptr;
}

static const char* test_atoi_zero() {
  const char* str = "0";
  int result = atoi(str);

  TEST_ASSERT_EQ(0, result, "atoi should convert zero");
  return nullptr;
}

// Test itoa function
static const char* test_itoa_positive() {
  char buffer[20];
  itoa(123, buffer, 10);

  TEST_ASSERT_STR_EQ("123", buffer, "itoa should convert positive number");
  return nullptr;
}

static const char* test_itoa_negative() {
  char buffer[20];
  itoa(-456, buffer, 10);

  TEST_ASSERT_STR_EQ("-456", buffer, "itoa should convert negative number");
  return nullptr;
}

static const char* test_itoa_zero() {
  char buffer[20];
  itoa(0, buffer, 10);

  TEST_ASSERT_STR_EQ("0", buffer, "itoa should convert zero");
  return nullptr;
}

// Test strtol function
static const char* test_strtol_basic() {
  const char* str = "12345";
  char* endptr;
  long result = strtol(str, &endptr, 10);

  TEST_ASSERT_EQ(12345, result, "strtol should convert string to long");
  TEST_ASSERT_EQ('\0', *endptr, "strtol should set end pointer correctly");
  return nullptr;
}

// Extended memory operations test
static const char* test_memmove_basic() {
  char buffer[20] = "1234567890";

  memmove(buffer + 3, buffer, 5);

  TEST_ASSERT_STR_EQ("1231234567890", buffer, "memmove should handle overlapping correctly");
  return nullptr;
}

// Test edge cases
static const char* test_strlen_null_terminated() {
  char buffer[10] = "hello";
  buffer[3] = '\0';

  size_t len = strlen(buffer);

  TEST_ASSERT_EQ(3, len, "strlen should stop at null terminator");
  return nullptr;
}

static const char* test_memcpy_partial() {
  char src[] = "1234567890";
  char dst[10];

  memcpy(dst, src, 3);
  dst[3] = '\0';

  TEST_ASSERT_STR_EQ("123", dst, "memcpy should copy partial string correctly");
  return nullptr;
}

// LibC string and memory test suite
static const test_case_t libc_tests[] = {
    // memcpy tests
    {"test_memcpy_basic", test_memcpy_basic},
    {"test_memcpy_zero_size", test_memcpy_zero_size},

    // memset tests
    {"test_memset_basic", test_memset_basic},
    {"test_memset_all", test_memset_all},

    // memcmp tests
    {"test_memcmp_equal", test_memcmp_equal},
    {"test_memcmp_different", test_memcmp_different},

    // strlen tests
    {"test_strlen_basic", test_strlen_basic},
    {"test_strlen_empty", test_strlen_empty},
    {"test_strlen_null_terminated", test_strlen_null_terminated},

    // strcpy tests
    {"test_strcpy_basic", test_strcpy_basic},

    // strcmp tests
    {"test_strcmp_equal", test_strcmp_equal},
    {"test_strcmp_less", test_strcmp_less},
    {"test_strcmp_greater", test_strcmp_greater},

    // strncmp tests
    {"test strncmp_basic", test strncmp_basic},

    // strchr tests
    {"test_strchr_found", test_strchr_found},
    {"test_strchr_not_found", test_strchr_not_found},

    // strrchr tests
    {"test_strrchr_found", test_strrchr_found},

    // strcat tests
    {"test_strcat_basic", test_strcat_basic},

    // strncpy tests
    {"test_strncpy_basic", test_strncpy_basic},

    // atoi tests
    {"test_atoi_positive", test_atoi_positive},
    {"test_atoi_negative", test_atoi_negative},
    {"test_atoi_zero", test_atoi_zero},

    // itoa tests
    {"test_itoa_positive", test_itoa_positive},
    {"test_itoa_negative", test_itoa_negative},
    {"test_itoa_zero", test_itoa_zero},

    // strtol tests
    {"test_strtol_basic", test_strtol_basic},

    // memmove tests
    {"test_memmove_basic", test_memmove_basic},
    {"test_memcpy_overlap", test_memcpy_overlap},
    {"test_memcpy_partial", test_memcpy_partial},
};

int run_libc_tests() {
  return run_tests(libc_tests, sizeof(libc_tests) / sizeof(libc_tests[0]));
}