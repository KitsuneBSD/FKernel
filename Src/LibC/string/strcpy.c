#include <LibC/string.h>

char *strcpy(char *dest, const char *src) {
  char *d = dest;
  for (size_t i = 0;; i++) {
    d[i] = src[i];
    if (src[i] == '\0') {
      break;
    }
  }
  return dest;
}
