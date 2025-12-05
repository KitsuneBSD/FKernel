#include <LibC/assert.h>
#include <LibC/string.h>

char *strrchr(const char *s, int c, size_t maxlen) {
  ASSERT(s != NULL);
  ASSERT(maxlen > 0);
  const char *last = NULL;
  for (size_t i = 0; i < maxlen; i++) {
    if (s[i] == '\0')
      break; // Reached null terminator before maxlen
    if (s[i] == (char)c)
      last = &s[i];
  }
  if ((char)c == '\0') { // Special case: finding the null terminator itself
    size_t len = strnlen(s, maxlen);
    if (len < maxlen)
      return (char *)&s[len]; // Return pointer to the null terminator
  }
  return (char *)last;
}
