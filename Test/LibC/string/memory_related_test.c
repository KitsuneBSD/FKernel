#include <Tests/LibC/string_test.h>

void memory_related_tests() {
  test_memmove();
  test_memcpy();
  test_memset();
  test_memcmp();
}
