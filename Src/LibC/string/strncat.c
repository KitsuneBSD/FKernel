#include <LibC/string.h>

char *strncat(char *dest, const char *src, size_t n) {
  char *d = dest + strlen(dest);
  while (n-- && *src)
    *d++ = *src++;
  *d = '\0';
  return dest;
}

char *strpbrk(const char *s, const char *accept) {
  for (; *s; ++s) {
    for (const char *a = accept; *a; ++a) {
      if (*s == *a) return (char *)s;
    }
  }
  return NULL;
}

size_t strspn(const char *s, const char *accept) {
  size_t count = 0;
  while (*s) {
    const char *a;
    for (a = accept; *a && *a != *s; ++a) {}
    if (!*a) break;
    ++count;
    ++s;
  }
  return count;
}

size_t strcspn(const char *s, const char *reject) {
  size_t count = 0;
  while (*s) {
    const char *r;
    for (r = reject; *r && *r != *s; ++r) {}
    if (*r) break;
    ++count;
    ++s;
  }
  return count;
}
