#pragma once

#include <stdio.h>

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"

static void check_result(const unsigned char *buffer,
                         const unsigned char *expected, int size,
                         const char *test_name) {
  int passed = 1;
  for (int i = 0; i < size; i++) {
    if (buffer[i] != expected[i]) {
      passed = 0;
      break;
    }
  }
  if (passed) {
    printf("%s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, test_name);
  } else {
    printf("%s[FAIL]%s %s\nExpected: ", COLOR_RED, COLOR_RESET, test_name);
    for (int i = 0; i < size; i++)
      printf("%02X ", expected[i]);
    printf("\nGot     : ");
    for (int i = 0; i < size; i++)
      printf("%02X ", buffer[i]);
    printf("\n");
  }
}

static void check_result_size(size_t got, size_t expected,
                              const char *test_name) {
  if (got == expected) {
    printf("%s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, test_name);
  } else {
    printf("%s[FAIL]%s %s\nExpected: %zu\nGot     : %zu\n", COLOR_RED,
           COLOR_RESET, test_name, expected, got);
  }
}

static void check_result_ptr(const char *got, const char *expected,
                             const char *test_name) {
  if (got == expected) {
    printf("%s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, test_name);
  } else {
    printf("%s[FAIL]%s %s\nExpected: %p\nGot     : %p\n", COLOR_RED,
           COLOR_RESET, test_name, (void *)expected, (void *)got);
  }
}

static void check_result_int(int got, int expected, const char *test_name) {
  if ((expected == 0 && got == 0) || (expected < 0 && got < 0) ||
      (expected > 0 && got > 0)) {
    printf("%s[PASS]%s %s (got %d)\n", COLOR_GREEN, COLOR_RESET, test_name,
           got);
  } else {
    printf("%s[FAIL]%s %s\nExpected: %d\nGot     : %d\n", COLOR_RED,
           COLOR_RESET, test_name, expected, got);
  }
}
