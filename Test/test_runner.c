#include <Tests/LibC/string_test.h>
#include <Tests/LibFK/container_test.h>
#include <Tests/LibFK/heap_malloc_test.h>
#include <Tests/LibFK/optional_test.h>
#include <Tests/LibFK/pair_test.h>
#include <Tests/LibFK/type_traits_test.h>
#include <stdio.h>

#define EXIT_SUCCESS 0

extern void test_pair(void);

int main(void) {
  printf("======================================== LibC "
         "=================================\n");
  string_test();

  printf("======================================== LibFK "
         "================================\n");
  test_pair();
  test_optional();
  test_type_traits();
  test_containers();
  test_heap_malloc();
  return EXIT_SUCCESS;
}
