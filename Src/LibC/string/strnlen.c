#include <LibC/string.h>

size_t strnlen(const char *s, size_t maxlen) {
  if (!s) return 0;
  size_t len = 0;
  while (len < maxlen && s[len] != '\0') {
    ++len;
  }
  return len;
}
