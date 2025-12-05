#include <LibC/assert.h>
#include <LibC/string.h>

char *strcpy(char *dest, const char *src) {
  ASSERT(dest != NULL);
  ASSERT(src != NULL);
  char *d = dest;
  size_t src_len = strlen(src);
  for (size_t i = 0; i <= src_len; i++) { // Include null terminator
    d[i] = src[i];
  }
  return dest;
}
