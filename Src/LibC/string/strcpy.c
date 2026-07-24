#include <LibC/string.h>

char *strcpy(char *dest, const char *src) {
  if (!dest || !src) return dest;
  char *d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}
