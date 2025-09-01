#include <Tests/LibC/string_test.h>
#include <stdio.h>

#define EXIT_SUCCESS 0

int main(void) {
  printf("======================================== LibC "
         "=================================\n");
  printf("================================ string.h "
         "=====================================\n");
  test_memmove();
  test_memcpy();
  test_memset();
  test_memcmp();
  printf("======================================== LibFK "
         "================================\n");
  return EXIT_SUCCESS;
}
