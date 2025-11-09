#include <LibC/stddef.h>
#include <LibC/string.h>

// TODO/FIXME: Simple bytewise memcpy implementation. Consider optimizing for
// alignment and larger word-sized copies (e.g., 32/64-bit) for performance. Also
// document behavior on overlapping ranges (this implementation does not handle
// overlap safely — use memmove for overlapping regions).
void *memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;

  for (size_t i = 0; i < n; ++i) {
    d[i] = s[i];
  }

  return dest;
}
