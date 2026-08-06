#include <tests/test_framework.h>
#include <LibC/string.h>
#include <LibC/stdarg.h>

// These tests validate the kernel's vsnprintf/snprintf implementation
// (LibC_Testing static lib, symbols renamed kernel_* to avoid clashing
// with the host libc).
extern "C" {
  int kernel_snprintf(char *str, size_t size, const char *fmt, ...);
  int kernel_vsnprintf(char *str, size_t size, const char *fmt, va_list args);
  int kernel_sscanf(const char *str, const char *fmt, ...);
}

// Wrapper that exercises kernel_vsnprintf through a real variadic boundary
static int call_vsnprintf(char *buffer, size_t size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int result = kernel_vsnprintf(buffer, size, fmt, args);
  va_end(args);
  return result;
}

// Test snprintf function
static const char* test_snprintf_basic() {
  char buffer[50];
  int result = kernel_snprintf(buffer, sizeof(buffer), "Hello %s", "world");

  TEST_ASSERT_EQ(11, result, "snprintf should return total characters written");
  TEST_ASSERT_STR_EQ("Hello world", buffer, "snprintf should format string correctly");
  return nullptr;
}

static const char* test_snprintf_numbers() {
  char buffer[50];
  int result = kernel_snprintf(buffer, sizeof(buffer), "Number: %d, Hex: 0x%x", 42, 42);

  TEST_ASSERT(result > 0, "snprintf should return positive result for numeric formatting");
  TEST_ASSERT_STR_EQ("Number: 42, Hex: 0x2a", buffer, "snprintf should format numbers correctly");
  return nullptr;
}

static const char* test_snprintf_truncated() {
  char buffer[10];
  int result = kernel_snprintf(buffer, sizeof(buffer), "This is a long string");

  TEST_ASSERT(result > (int)sizeof(buffer) - 1, "snprintf should indicate truncation");
  TEST_ASSERT(strlen(buffer) < sizeof(buffer), "buffer should be null-terminated");
  return nullptr;
}

static const char* test_snprintf_zero_size() {
  int result = kernel_snprintf(nullptr, 0, "Hello %s", "world");

  TEST_ASSERT_EQ(11, result, "snprintf with zero size should return required space");
  return nullptr;
}

// Test vsnprintf function
static const char* test_vsnprintf_basic() {
  char buffer[50];
  int result = call_vsnprintf(buffer, sizeof(buffer), "Number: %d", 42);

  TEST_ASSERT(result > 0, "vsnprintf formatting should work");
  TEST_ASSERT_STR_EQ("Number: 42", buffer, "vsnprintf should format correctly");
  return nullptr;
}

static const char* test_vsnprintf_formatting() {
  char buffer[100];

  // Test multiple format specifiers
  int result =
      call_vsnprintf(buffer, sizeof(buffer), "String: %s, Char: %c, Int: %d, Hex: 0x%x, Pointer: %p",
                     "test", 'A', 123, 123, buffer);

  TEST_ASSERT(result > 0, "vsnprintf should handle multiple specifiers");
  TEST_ASSERT(strncmp(buffer, "String: test, Char: A", 21) == 0,
              "vsnprintf should format string part correctly");
  return nullptr;
}

static const char* test_vsnprintf_precision() {
  char buffer[50];

  // Test precision with strings
  int result = call_vsnprintf(buffer, sizeof(buffer), "%.3s", "hello world");

  TEST_ASSERT_EQ(3, result, "vsnprintf precision should limit output");
  TEST_ASSERT_STR_EQ("hel", buffer, "vsnprintf should respect precision");
  return nullptr;
}

// Test format specifiers
static const char* test_format_decimal() {
  char buffer[50];

  // Test positive, negative, and zero
  kernel_snprintf(buffer, sizeof(buffer), "%d", 42);
  TEST_ASSERT_STR_EQ("42", buffer, "format decimal positive");

  kernel_snprintf(buffer, sizeof(buffer), "%d", -42);
  TEST_ASSERT_STR_EQ("-42", buffer, "format decimal negative");

  kernel_snprintf(buffer, sizeof(buffer), "%d", 0);
  TEST_ASSERT_STR_EQ("0", buffer, "format decimal zero");

  return nullptr;
}

static const char* test_format_int64_min() {
  char buffer[40];

  // INT64_MIN is the canonical signed-overflow trap: -0x8000000000000000.
  // Magnitude must be computed as unsigned (0 - (uint64_t)val).
  kernel_snprintf(buffer, sizeof(buffer), "%lld", (long long)INT64_MIN);
  TEST_ASSERT_STR_EQ("-9223372036854775808", buffer, "format INT64_MIN without overflow");

  kernel_snprintf(buffer, sizeof(buffer), "%ld", (long)INT32_MIN);
  TEST_ASSERT_STR_EQ("-2147483648", buffer, "format INT32_MIN without overflow");

  return nullptr;
}

static const char* test_format_hexadecimal() {
  char buffer[50];

  // Test lowercase hex
  kernel_snprintf(buffer, sizeof(buffer), "%x", 255);
  TEST_ASSERT_STR_EQ("ff", buffer, "format hex lowercase");

  // Test uppercase hex
  kernel_snprintf(buffer, sizeof(buffer), "%X", 255);
  TEST_ASSERT_STR_EQ("FF", buffer, "format hex uppercase");

  // Test hex with 0x prefix
  kernel_snprintf(buffer, sizeof(buffer), "0x%x", 42);
  TEST_ASSERT_STR_EQ("0x2a", buffer, "format hex with prefix");

  return nullptr;
}

static const char* test_format_octal() {
  char buffer[50];

  kernel_snprintf(buffer, sizeof(buffer), "%o", 8);
  TEST_ASSERT_STR_EQ("10", buffer, "format octal");

  return nullptr;
}

static const char* test_format_character() {
  char buffer[50];

  kernel_snprintf(buffer, sizeof(buffer), "%c", 'A');
  TEST_ASSERT_STR_EQ("A", buffer, "format character");

  kernel_snprintf(buffer, sizeof(buffer), "%c", '\n');
  TEST_ASSERT_EQ('\n', buffer[0], "format newline character");

  return nullptr;
}

static const char* test_format_string() {
  char buffer[50];

  kernel_snprintf(buffer, sizeof(buffer), "%s", "hello");
  TEST_ASSERT_STR_EQ("hello", buffer, "format string");

  kernel_snprintf(buffer, sizeof(buffer), "%s", "");
  TEST_ASSERT_STR_EQ("", buffer, "format empty string");

  return nullptr;
}

static const char* test_format_pointer() {
  char buffer[50];
  void* ptr = (void*)0x12345678;

  // %p must NOT impose a default width/zero-padding; output is just 0x + hex.
  int result = kernel_snprintf(buffer, sizeof(buffer), "%p", ptr);
  TEST_ASSERT_EQ(10, result, "format pointer should be 0x + 8 hex digits");
  TEST_ASSERT_STR_EQ("0x12345678", buffer, "format pointer should start with 0x");

  return nullptr;
}

// Test width and flags
static const char* test_format_width() {
  char buffer[50];

  // Test minimum width
  kernel_snprintf(buffer, sizeof(buffer), "%10s", "hello");
  TEST_ASSERT_EQ(10, strlen(buffer), "format should respect width");

  // Test zero padding
  kernel_snprintf(buffer, sizeof(buffer), "%05d", 42);
  TEST_ASSERT_STR_EQ("00042", buffer, "format should zero pad");

  // Test left justify
  kernel_snprintf(buffer, sizeof(buffer), "%-5s", "hi");
  TEST_ASSERT_STR_EQ("hi   ", buffer, "format should left justify");

  return nullptr;
}

static const char* test_format_special_cases() {
  char buffer[50];

  // Test percent sign
  kernel_snprintf(buffer, sizeof(buffer), "%%");
  TEST_ASSERT_STR_EQ("%", buffer, "format percent sign");

  // Test null string pointer (should show "(null)")
  kernel_snprintf(buffer, sizeof(buffer), "%s", (char*)nullptr);
  TEST_ASSERT(strncmp(buffer, "(null)", 6) == 0 || strncmp(buffer, "0x", 2) == 0,
              "format null pointer should handle gracefully");

  return nullptr;
}

// Edge cases and stress tests
static const char* test_edge_cases() {
  char buffer[5];

  // Test buffer exactly filled
  int result = kernel_snprintf(buffer, 5, "1234");
  TEST_ASSERT_EQ(4, result, "should return characters written");
  TEST_ASSERT_STR_EQ("1234", buffer, "should fill buffer exactly");

  // Test buffer overflow protection
  result = kernel_snprintf(buffer, 5, "123456");
  TEST_ASSERT_EQ(6, result, "should indicate truncation occurred");
  TEST_ASSERT_STR_EQ("1234", buffer, "should truncate safely");

  return nullptr;
}

// L10: vsnprintf precision for integer types
static const char* test_vsnprintf_precision_integer() {
  char buffer[50];

  // %.5d: minimum 5 digits
  int result = kernel_snprintf(buffer, sizeof(buffer), "%.5d", 42);
  TEST_ASSERT_EQ(5, result, "%.5d should produce 5 chars");
  TEST_ASSERT_STR_EQ("00042", buffer, "%.5d should zero-pad to 5 digits");

  // %.5d with value already >= 5 digits
  kernel_snprintf(buffer, sizeof(buffer), "%.5d", 123456);
  TEST_ASSERT_STR_EQ("123456", buffer, "%.5d should not truncate wider value");

  // %.5d with negative
  kernel_snprintf(buffer, sizeof(buffer), "%.5d", -42);
  TEST_ASSERT_STR_EQ("-00042", buffer, "%.5d negative should zero-pad digits");

  // width + precision: %8.5d
  kernel_snprintf(buffer, sizeof(buffer), "%8.5d", 42);
  TEST_ASSERT_STR_EQ("   00042", buffer, "%8.5d should space-pad to total width 8");

  // '0' flag ignored when precision specified: %08.5d same as %8.5d
  kernel_snprintf(buffer, sizeof(buffer), "%08.5d", 42);
  TEST_ASSERT_STR_EQ("   00042", buffer, "%08.5d: '0' flag ignored when precision given");

  // %.5x hex
  kernel_snprintf(buffer, sizeof(buffer), "%.5x", 0xff);
  TEST_ASSERT_STR_EQ("000ff", buffer, "%.5x should zero-pad hex to 5 digits");

  return nullptr;
}

// L9: vsscanf matching failure — %i/%d with no digits must not count as match
static const char* test_sscanf_matching_failure() {
  int x = 99;

  // %i with purely non-digit input: matching failure → returns EOF (< 0)
  int ret = kernel_sscanf("abc", "%i", &x);
  TEST_ASSERT(ret < 0, "%i with non-digit input must return EOF");
  TEST_ASSERT_EQ(99, x, "%i matching failure must not write to output");

  // %d with non-digit input: matching failure
  x = 99;
  ret = kernel_sscanf("abc", "%d", &x);
  TEST_ASSERT(ret < 0, "%d with non-digit input must return EOF");
  TEST_ASSERT_EQ(99, x, "%d matching failure must not write to output");

  // %i with "0" prefix consumed: value 0, counts as match
  x = 99;
  ret = kernel_sscanf("0abc", "%i", &x);
  TEST_ASSERT_EQ(1, ret, "%i with '0' prefix should match (value 0)");
  TEST_ASSERT_EQ(0, x, "%i with '0' prefix should produce value 0");

  // %d with sign then no digits: matching failure, backtrack
  x = 99;
  ret = kernel_sscanf("-abc", "%d", &x);
  TEST_ASSERT(ret < 0, "%d with sign then no digits must return EOF");
  TEST_ASSERT_EQ(99, x, "%d sign+no-digits must not write to output");

  // Normal %d: must still work
  x = 99;
  ret = kernel_sscanf("42", "%d", &x);
  TEST_ASSERT_EQ(1, ret, "%d with valid digits must match");
  TEST_ASSERT_EQ(42, x, "%d must parse correct value");

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
    {"test_format_int64_min", test_format_int64_min},
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

    // L10: precision for integers
    {"test_vsnprintf_precision_integer", test_vsnprintf_precision_integer},

    // L9: sscanf matching failure
    {"test_sscanf_matching_failure", test_sscanf_matching_failure},
};

int run_libc_stdio_tests() {
  return run_tests("LibC::Stdio", stdio_tests, sizeof(stdio_tests) / sizeof(stdio_tests[0]));
}
