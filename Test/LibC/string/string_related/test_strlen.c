#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>
#include <assert.h>

static void test_strlen_empty() {
  check_result_size(strlen(""), 0, "strlen_empty");
}

static void test_strlen_single_char() {
  check_result_size(strlen("A"), 1, "strlen_single_char");
}

static void test_strlen_simple_word() {
  check_result_size(strlen("hello"), 5, "strlen_simple_word");
}

static void test_strlen_with_space() {
  check_result_size(strlen("with space"), 10, "strlen_with_space");
}

static void test_strlen_with_newline() {
  check_result_size(strlen("line\nbreak"), 10, "strlen_with_newline");
}

static void test_strlen_embedded_null() {
  const char str[] = {'n', 'u', 'l', 'l', '\0', 'h',
                      'i', 'd', 'd', 'e', 'n',  '\0'};
  check_result_size(strlen(str), 4, "strlen_embedded_null");
}

void test_strlen() {
  test_strlen_empty();
  test_strlen_single_char();
  test_strlen_simple_word();
  test_strlen_with_space();
  test_strlen_with_newline();
  test_strlen_embedded_null();

  printf("All strlen unit tests finished.\n");
}
