#include <LibC/stddef.h>
#include <LibC/string.h>

char *strchr(const char *s, int c) {
  unsigned char target = (unsigned char)c;
  while (1) {
    if ((unsigned char)*s == target)
      return (char *)s;
    if (*s == '\0')
      return NULL;
    s++;
  }
}
