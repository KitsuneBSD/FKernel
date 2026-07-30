#include <LibC/stddef.h>
#include <LibC/string.h>

void *memmove(void *dest, const void *src, size_t n) {
  if ((n == 0) || (dest == src))
    return dest;

  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;

  if (d < s) {
    __asm__ volatile("rep movsb"
                     : "+D"(d), "+S"(s), "+c"(n)
                     :
                     : "memory");
  } else {
    /* Copy backwards: set DF, start from last byte, then clear DF */
    d += n - 1;
    s += n - 1;
    __asm__ volatile("std; rep movsb; cld"
                     : "+D"(d), "+S"(s), "+c"(n)
                     :
                     : "memory");
  }
  return dest;
}
