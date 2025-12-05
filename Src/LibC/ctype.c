#include <LibC/assert.h>
#include <LibC/ctype.h>
#include <LibC/string.h>

int to_upper(int c) {
  ASSERT(c >= 0 && c <= 127); // to_upper needs to run on ascii code

  if (c >= 'a' && c <= 'z')
    return c - ('a' - 'A');

  return c;
}

int to_lower(int c) {
  ASSERT(c >= 0 && c <= 127); // to_lower needs to run on ascii code

  if (c >= 'A' && c <= 'Z')
    return c + ('a' - 'A');

  return c;
}

void capitalize(char *str, size_t maxlen) {
  ASSERT(str != NULL);
  ASSERT(maxlen > 0); // Ensure maxlen is positive
  ASSERT(strnlen(str, maxlen) > 0); // Ensure string is not empty within maxlen

  str[0] = to_upper(str[0]);

  for (size_t i = 1; i < maxlen && str[i] != ' '; i++) {
    str[i] = to_lower(str[i]);
  }
}
