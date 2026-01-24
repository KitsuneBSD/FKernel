#include <LibC/assert.h>
#include <LibC/string.h>

size_t strnlen(const char *s, size_t maxlen) {
  ASSERT(s != NULL);
  ASSERT(maxlen > 0); // maxlen must be positive
  size_t len = 0;
  while (len < maxlen && s[len] != '\0') {
    ++len;
  }
  return len;
}
