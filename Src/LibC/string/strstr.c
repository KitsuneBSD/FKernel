#include <LibC/string.h>

char *strstr(const char *haystack, const char *needle) {
  if (!needle || needle[0] == '\0') return (char *)haystack;
  size_t needle_len = strlen(needle);
  for (; *haystack; ++haystack) {
    if (strncmp(haystack, needle, needle_len) == 0)
      return (char *)haystack;
  }
  return NULL;
}
