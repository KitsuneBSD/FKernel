#include <LibC/assert.h>
#include <LibC/string.h>

int strncmp(const char *s1, const char *s2, size_t n) {
  ASSERT(s1 != NULL);
  ASSERT(s2 != NULL);
  if (n == 0)
    return 0;

  while (n-- && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  if (n == (size_t)-1)
    return 0;

  return (unsigned char)*s1 - (unsigned char)*s2;
}
