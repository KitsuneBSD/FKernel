#include <Tests/LibC/string_test.h>

void string_related_tests() {
  test_strcpy();
  test_strcmp();
  test_strncpy();
  test_strchr();
  test_strcat();
  test_strlen();
}
