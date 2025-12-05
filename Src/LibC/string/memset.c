#include <LibC/assert.h>
#include <LibC/stddef.h>
#include <LibC/string.h>

void *memset(void *s, int c, size_t n) {
  ASSERT(s != NULL); // Assert source pointer is not null
  unsigned char *p = (unsigned char *)s;
  ASSERT(p != NULL); // Assert casted pointer is not null
  unsigned char byte = (unsigned char)c;

  for (size_t i = 0; i < n; ++i) {
    p[i] = byte;
  }

  return s;
}
