#include <LibC/assert.h>
#include <LibC/stddef.h>
#include <LibC/string.h>

char *strncpy(char *dest, const char *src, size_t n) {
  ASSERT(dest != NULL);
  ASSERT(src != NULL);
  size_t i = 0;
  for (; i < n && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }
  for (; i < n; i++) {
    dest[i] = '\0';
  }
  return dest;
}
