#include <LibC/stddef.h>
#include <LibC/string.h>

char *strrnchr(const char *s, int c, size_t maxlen) {
  const char *last = NULL;
  unsigned char target = (unsigned char)c;
  for (size_t i = 0; i < maxlen; i++) {
    if (s[i] == '\0')
      break;
    if ((unsigned char)s[i] == target)
      last = &s[i];
  }
  if ((unsigned char)c == '\0') {
    size_t len = strnlen(s, maxlen);
    if (len < maxlen)
      return (char *)&s[len];
  }
  return (char *)last;
}

char *strrchr(const char *s, int c) {
  const char *last = NULL;
  unsigned char target = (unsigned char)c;
  while (*s) {
    if ((unsigned char)*s == target)
      last = s;
    s++;
  }
  if (target == '\0')
    return (char *)s;
  return (char *)last;
}
