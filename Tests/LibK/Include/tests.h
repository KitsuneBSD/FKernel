#pragma once

#include <stdio.h>

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"

static int tests_run = 0;
static int tests_failed = 0;

#define TEST_MSG(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

#define ASSERT_TRUE(cond)                                                      \
  do {                                                                         \
    tests_run++;                                                               \
    if (!(cond)) {                                                             \
      fprintf(stderr,                                                          \
              COLOR_RED "Assertion failed: %s\n"                               \
                        "  Location: %s:%d\n" COLOR_RESET,                     \
              #cond, __FILE__, __LINE__);                                      \
      tests_failed++;                                                          \
    } else {                                                                   \
      fprintf(stderr,                                                          \
              COLOR_GREEN "Assertion passed: %s\n"                             \
                          "  Location: %s:%d\n" COLOR_RESET,                   \
              #cond, __FILE__, __LINE__);                                      \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(val1, val2)                                                  \
  do {                                                                         \
    tests_run++;                                                               \
    if ((val1) != (val2)) {                                                    \
      fprintf(stderr,                                                          \
              COLOR_RED "Assertion failed: %s == %s\n"                         \
                        "  Location: %s:%d\n"                                  \
                        "  Got: %ld != %ld\n" COLOR_RESET,                     \
              #val1, #val2, __FILE__, __LINE__, (long)(val1), (long)(val2));   \
      tests_failed++;                                                          \
    } else {                                                                   \
      fprintf(stderr,                                                          \
              COLOR_GREEN "Assertion passed: %s == %s\n"                       \
                          "  Location: %s:%d\n" COLOR_RESET,                   \
              #val1, #val2, __FILE__, __LINE__);                               \
    }                                                                          \
  } while (0)

#define RUN_TEST(test_func)                                                    \
  do {                                                                         \
    printf(COLOR_YELLOW "Running test: %s...\n" COLOR_RESET, #test_func);      \
    test_func();                                                               \
  } while (0)

static void print_test_summary(void) {
  printf("\nTotal tests run: %d\n", tests_run);
  if (tests_failed > 0) {
    printf(COLOR_RED "Tests failed: %d\n" COLOR_RESET, tests_failed);
  } else {
    printf(COLOR_GREEN "All tests passed successfully!\n" COLOR_RESET);
  }
}
