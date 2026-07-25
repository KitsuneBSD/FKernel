#include <LibC/stddef.h>
#include <LibC/string.h>

void *memcpy(void *dest, const void *src, size_t n) {
  void *orig = dest;
  __asm__ volatile(
    "rep movsb"
    : "+D"(dest), "+S"(src), "+c"(n)
    :
    : "memory"
  );
  return orig;
}
