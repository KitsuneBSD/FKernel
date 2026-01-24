#include <LibC/assert.h>
#include <LibC/stddef.h>
#include <LibC/string.h>

size_t strlen(const char *s) {
  ASSERT(s != NULL);
  return strnlen(s, (size_t)-1); // Use SIZE_MAX as the bound for strnlen
}
