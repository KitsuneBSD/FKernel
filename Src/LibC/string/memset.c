#include <LibC/stddef.h>
#include <LibC/string.h>

void *memset(void *s, int c, size_t n) {
  void *orig = s;
  __asm__ volatile(
    "rep stosb"
    : "+D"(s), "+c"(n)
    : "a"((unsigned char)c)
    : "memory"
  );
  return orig;
}
