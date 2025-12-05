#include <LibC/assert.h>
#include <LibC/string.h>

int strcmp(const char *s1, const char *s2) {
  ASSERT(s1 != NULL);
  ASSERT(s2 != NULL);
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  size_t maxlen = (len1 > len2)
                      ? len1
                      : len2; // Compare up to the length of the longer string
  return strncmp(s1, s2,
                 maxlen + 1); // +1 to include null terminator in comparison
}
