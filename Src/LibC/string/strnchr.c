#include <LibC/assert.h>
#include <LibC/stddef.h>
#include <LibC/string.h>

char *strnchr(const char *s, int c, size_t n) {
  ASSERT(s != NULL);
  ASSERT(n >= 0); // n is size_t, so it's always >= 0. But good for clarity.
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
