#include <Tests/LibC/string_test.h>
#include <stdio.h>

void string_test() {

  printf("================================ string.h "
         "=====================================\n");

  memory_related_tests();
  string_related_tests();
}
