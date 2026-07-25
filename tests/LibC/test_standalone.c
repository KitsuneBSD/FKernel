#include <LibC/stddef.h>
#include <LibC/stdint.h>

// Simple standalone test implementation
void* test_memcpy(void* dest, const void* src, size_t n) {
  char* d = (char*)dest;
  const char* s = (const char*)src;

  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }

  return dest;
}

void* test_memset(void* s, int c, size_t n) {
  char* p = (char*)s;
  for (size_t i = 0; i < n; i++) {
    p[i] = (char)c;
  }
  return s;
}

size_t test_strlen(const char* s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

char* test_strcpy(char* dest, const char* src) {
  char* d = dest;
  while ((*d++ = *src++) != '\0') {
    // Copy characters
  }
  return dest;
}

int test_strcmp(const char* s1, const char* s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Test assertions
#define TEST_ASSERT(condition)                                                                     \
  do {                                                                                             \
    if (!(condition)) {                                                                            \
      return 1;                                                                                    \
    }                                                                                              \
  } while (0)

#define TEST_ASSERT_EQ(a, b)                                                                       \
  do {                                                                                             \
    if ((a) != (b)) {                                                                              \
      return 1;                                                                                    \
    }                                                                                              \
  } while (0)

// Test functions
static int test_memcpy_function() {
  char src[10] = "hello";
  char dst[10];

  test_memcpy(dst, src, 6);

  for (int i = 0; i < 6; i++) {
    TEST_ASSERT_EQ(dst[i], src[i]);
  }

  return 0;
}

static int test_memset_function() {
  char buffer[10];

  test_memset(buffer, 'A', 5);

  for (int i = 0; i < 5; i++) {
    TEST_ASSERT_EQ(buffer[i], 'A');
  }

  return 0;
}

static int test_strlen_function() {
  const char* str = "hello";
  size_t len = test_strlen(str);

  TEST_ASSERT_EQ(len, 5);
  return 0;
}

static int test_strcpy_function() {
  const char* src = "test";
  char dst[10];

  test_strcpy(dst, src);

  for (int i = 0; i < 4; i++) {
    TEST_ASSERT_EQ(dst[i], src[i]);
  }

  TEST_ASSERT_EQ(dst[4], '\0');
  return 0;
}

static int test_strcmp_function() {
  const char* str1 = "hello";
  const char* str2 = "hello";
  const char* str3 = "world";

  TEST_ASSERT_EQ(test_strcmp(str1, str2), 0);
  TEST_ASSERT(test_strcmp(str1, str3) != 0);

  return 0;
}

// Test runner
int main() {
  int failed = 0;

  if (test_memcpy_function() != 0)
    failed++;
  if (test_memset_function() != 0)
    failed++;
  if (test_strlen_function() != 0)
    failed++;
  if (test_strcpy_function() != 0)
    failed++;
  if (test_strcmp_function() != 0)
    failed++;

  return failed;
}