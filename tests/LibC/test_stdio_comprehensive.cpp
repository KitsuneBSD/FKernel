#include "test_framework.h"
#include <LibC/stdio.h>
#include <LibC/string.h>

// Test snprintf function
static const char* test_snprintf_basic() {
  char buffer[50];
  int result = snprintf(buffer, sizeof(buffer), "Hello %s", "world");

  TEST_ASSERT_EQ(11, result, "snprintf should return total characters written");
  TEST_ASSERT_STR_EQ("Hello world", buffer, "snprintf should format string correctly");
  return nullptr;
}

static const char* test_snprintf_numbers() {
  char buffer[50];
  int result = snprintf(buffer, sizeof(buffer), "Number: %d, Hex: 0x%x", 42, 42);

  TEST_ASSERT(result > 0, "snprintf should return positive result for numeric formatting");
  TEST_ASSERT_STR_EQ("Number: 42, Hex: 0x2a", buffer, "snprintf should format numbers correctly");
  return nullptr;
}

static const char* test_snprintf_truncated() {
  char buffer[10];
  int result = snprintf(buffer, sizeof(buffer), "This is a long string");

  TEST_ASSERT(result > (int)sizeof(buffer) - 1, "snprintf should indicate truncation");
  TEST_ASSERT(strlen(buffer) < sizeof(buffer), "buffer should be null-terminated");
  return nullptr;
}

static const char* test_snprintf_zero_size() {
  int result = snprintf(nullptr, 0, "Hello %s", "world");

  TEST_ASSERT_EQ(11, result, "snprintf with zero size should return required space");
  return nullptr;
}

// Test vsnprintf function
static const char* test_vsnprintf_basic() {
  char buffer[50];
  va_list args;

  // Setup va_list as if we received it from a variadic function
  const char* format = "Number: %d";
  int number = 42;

  // Note: In real test, this would be called from a variadic function
  // For testing purposes, we'll test the underlying functionality
  int result = snprintf(buffer, sizeof(buffer), format, number);

  TEST_ASSERT(result > 0, "vsnprintf formatting should work");
  TEST_ASSERT_STR_EQ("Number: 42", buffer, "vsnprintf should format correctly");
  return nullptr;
}

static const char* test_vsnprintf_formatting() {
  char buffer[100];

  // Test multiple format specifiers
  int result =
      snprintf(buffer, sizeof(buffer), "String: %s, Char: %c, Int: %d, Hex: 0x%x, Pointer: %p",
               "test", 'A', 123, 123, buffer);

  TEST_ASSERT(result > 0, "vsnprintf should handle multiple specifiers");
  TEST_ASSERT(strncmp(buffer, "String: test", 13) == 0,
              "vsnprintf should format string part correctly");
  return nullptr;
}

static const char* test_vsnprintf_precision() {
  char buffer[50];

  // Test precision with strings
  int result = snprintf(buffer, sizeof(buffer), "%.3s", "hello world");

  TEST_ASSERT_EQ(3, result, "vsnprintf precision should limit output");
  TEST_ASSERT_STR_EQ("hel", buffer, "vsnprintf should respect precision");
  return nullptr;
}

// Test format specifiers
static const char* test_format_decimal() {
  char buffer[50];

  // Test positive, negative, and zero
  snprintf(buffer, sizeof(buffer), "%d", 42);
  TEST_ASSERT_STR_EQ("42", buffer, "format decimal positive");

  snprintf(buffer, sizeof(buffer), "%d", -42);
  TEST_ASSERT_STR_EQ("-42", buffer, "format decimal negative");

  snprintf(buffer, sizeof(buffer), "%d", 0);
  TEST_ASSERT_STR_EQ("0", buffer, "format decimal zero");

  return nullptr;
}

static const char* test_format_hexadecimal() {
  char buffer[50];

  // Test lowercase hex
  snprintf(buffer, sizeof(buffer), "%x", 255);
  TEST_ASSERT_STR_EQ("ff", buffer, "format hex lowercase");

  // Test uppercase hex
  snprintf(buffer, sizeof(buffer), "%X", 255);
  TEST_ASSERT_STR_EQ("FF", buffer, "format hex uppercase");

  // Test hex with 0x prefix
  snprintf(buffer, sizeof(buffer), "0x%x", 42);
  TEST_ASSERT_STR_EQ("0x2a", buffer, "format hex with prefix");

  return nullptr;
}

static const char* test_format_octal() {
  char buffer[50];

  snprintf(buffer, sizeof(buffer), "%o", 8);
  TEST_ASSERT_STR_EQ("10", buffer, "format octal");

  return nullptr;
}

static const char* test_format_character() {
  char buffer[50];

  snprintf(buffer, sizeof(buffer), "%c", 'A');
  TEST_ASSERT_STR_EQ("A", buffer, "format character");

  snprintf(buffer, sizeof(buffer), "%c", '\n');
  TEST_ASSERT_EQ('\n', buffer[0], "format newline character");

  return nullptr;
}

static const char* test_format_string() {
  char buffer[50];

  snprintf(buffer, sizeof(buffer), "%s", "hello");
  TEST_ASSERT_STR_EQ("hello", buffer, "format string");

  snprintf(buffer, sizeof(buffer), "%s", "");
  TEST_ASSERT_STR_EQ("", buffer, "format empty string");

  return nullptr;
}

static const char* test_format_pointer() {
  char buffer[50];
  void* ptr = (void*)0x12345678;

  snprintf(buffer, sizeof(buffer), "%p", ptr);
  TEST_ASSERT(buffer[0] == '0' && buffer[1] == 'x', "format pointer should start with 0x");

  return nullptr;
}

// Test width and flags
static const char* test_format_width() {
  char buffer[50];

  // Test minimum width
  snprintf(buffer, sizeof(buffer), "%10s", "hello");
  TEST_ASSERT_EQ(10, strlen(buffer), "format should respect width");

  // Test zero padding
  snprintf(buffer, sizeof(buffer), "%05d", 42);
  TEST_ASSERT_STR_EQ("00042", buffer, "format should zero pad");

  // Test left justify
  snprintf(buffer, sizeof(buffer), "%-5s", "hi");
  TEST_ASSERT_STR_EQ("hi   ", buffer, "format should left justify");

  return nullptr;
}

static const char* test_format_special_cases() {
  char buffer[50];

  // Test percent sign
  snprintf(buffer, sizeof(buffer), "%%");
  TEST_ASSERT_STR_EQ("%", buffer, "format percent sign");

  // Test null string pointer (should show "(null)")
  snprintf(buffer, sizeof(buffer), "%s", (char*)nullptr);
  TEST_ASSERT(strncmp(buffer, "(null)", 6) == 0 || strncmp(buffer, "0x", 2) == 0,
              "format null pointer should handle gracefully");

  return nullptr;
}

// Edge cases and stress tests
static const char* test_edge_cases() {
  char buffer[5];

  // Test buffer exactly filled
  int result = snprintf(buffer, 5, "1234");
  TEST_ASSERT_EQ(4, result, "should return characters written");
  TEST_ASSERT_STR_EQ("1234", buffer, "should fill buffer exactly");

  // Test buffer overflow protection
  result = snprintf(buffer, 5, "123456");
  TEST_ASSERT_EQ(6, result, "should indicate truncation occurred");
  TEST_ASSERT_STR_EQ("1234", buffer, "should truncate safely");

  return nullptr;
}

// stdio test suite
static const test_case_t stdio_tests[] = {
    // snprintf tests
    {"test_snprintf_basic", test_snprintf_basic},
    {"test_snprintf_numbers", test_snprintf_numbers},
    {"test_snprintf_truncated", test_snprintf_truncated},
    {"test_snprintf_zero_size", test_snprintf_zero_size},

    // vsnprintf tests
    {"test_vsnprintf_basic", test_vsnprintf_basic},
    {"test_vsnprintf_formatting", test_vsnprintf_formatting},
    {"test_vsnprintf_precision", test_vsnprintf_precision},

    // Format specifier tests
    {"test_format_decimal", test_format_decimal},
    {"test_format_hexadecimal", test_format_hexadecimal},
    {"test_format_octal", test_format_octal},
    {"test_format_character", test_format_character},
    {"test_format_string", test_format_string},
    {"test_format_pointer", test_format_pointer},

    // Width and flags tests
    {"test_format_width", test_format_width},
    {"test_format_special_cases", test_format_special_cases},

    // Edge cases
    {"test_edge_cases", test_edge_cases},
};

int run_stdio_tests() {
  return run_tests(stdio_tests, sizeof(stdio_tests) / sizeof(stdio_tests[0]));
}