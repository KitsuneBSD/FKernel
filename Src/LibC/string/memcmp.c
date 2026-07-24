#include <LibC/string.h>

int memcmp(const void *s1, const void *s2, size_t n) {
  if (!s1 || !s2) return -1;
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;

  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return (int)(p1[i] - p2[i]);
    }
  }
  return 0;
}
