#include <LibC/string.h>

char *strcat(char *dest, const char *src) {
  if (!dest || !src) return dest;
  size_t dest_len = strlen(dest);
  size_t src_len = strlen(src);
  for (size_t j = 0; j < src_len; j++) {
    dest[dest_len + j] = src[j];
  }
  dest[dest_len + src_len] = '\0';
  return dest;
}
