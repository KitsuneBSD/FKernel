#include <LibC/stddef.h>
#include <LibC/string.h>

size_t strlen(const char *s) {
  int sum = 0;
  for (int i = 0; s[i] != '\0'; ++i) {
    sum++;
  }

  return sum;
}
