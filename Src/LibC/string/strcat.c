#include <LibC/stddef.h>
#include <LibC/string.h>

char *strcat(char *dest, const char *src) {
  size_t i, j;
  for (i = 0; dest[i] != '\0'; i++)
    ;
  for (j = 0;; j++) {
    dest[i + j] = src[j];
    if (src[j] == '\0') {
      break;
    }
  }
  return dest;
}
