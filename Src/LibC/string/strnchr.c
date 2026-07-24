#include <LibC/stddef.h>
#include <LibC/string.h>

char *strnchr(const char *s, int c, size_t n) {
  unsigned char target = (unsigned char)c;
  for (size_t i = 0; i < n; i++) {
    if (s[i] == target) {
      return (char *)&s[i];
    }
    if (s[i] == '\0') {
      return NULL;
    }
  }
  return NULL;
}
