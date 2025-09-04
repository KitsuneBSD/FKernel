#include <LibC/stddef.h>
#include <LibC/string.h>

char *strchr(const char *s, int c) {
  unsigned char target = (unsigned char)c;
  for (size_t i = 0;; i++) {
    if (s[i] == target) {
      return (char *)&s[i];
    }
    if (s[i] == '\0') {
      return NULL;
    }
  }
}
