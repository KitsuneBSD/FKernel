#pragma once

#include <stdio.h>

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
    printf("[PASS] %s\n", test_name);
  } else {
    printf("[FAIL] %s\nExpected: ", test_name);
    for (int i = 0; i < size; i++)
      printf("%02X ", expected[i]);
    printf("\nGot     : ");
    for (int i = 0; i < size; i++)
      printf("%02X ", buffer[i]);
    printf("\n");
  }
}
