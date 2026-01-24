#include <LibC/assert.h>
#include <LibC/stddef.h>
#include <LibC/string.h>

char *strchr(const char *s, int c, size_t maxlen) {
  ASSERT(s != NULL);
  ASSERT(maxlen > 0);
  unsigned char target = (unsigned char)c;
  for (size_t i = 0; i < maxlen; i++) {
    if (s[i] == target) {
      return (char *)&s[i];
    }
    if (s[i] == '\0') {
      return NULL;
    }
  }
  return NULL; // Not found within maxlen
}
