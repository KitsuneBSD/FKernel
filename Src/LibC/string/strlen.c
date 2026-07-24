#include <LibC/string.h>

size_t strlen(const char *s) {
  if (!s) return 0;
  return strnlen(s, (size_t)-1);
}
