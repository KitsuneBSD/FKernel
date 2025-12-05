#include <LibC/assert.h>
#include <LibC/stddef.h>
#include <LibC/string.h>

char *strcat(char *dest, const char *src) {
  ASSERT(dest != NULL);
  ASSERT(src != NULL);
  size_t dest_len = strlen(dest);
  size_t src_len = strlen(src);
  // Assuming dest has enough space. For safety, strncat should be preferred.
  for (size_t j = 0; j < src_len; j++) {
    dest[dest_len + j] = src[j];
  }
  dest[dest_len + src_len] = '\0';
  return dest;
}
