#include <Tests/LibC/string_test.h>
#include <stdio.h>

#define EXIT_SUCCESS 0

int main(void) {
  printf("======================================== LibC "
         "=================================\n");
  string_test();
  printf("======================================== LibFK "
         "================================\n");
  return EXIT_SUCCESS;
}
