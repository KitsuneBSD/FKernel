#include <LibC/assert.h>
#include <LibC/ctype.h>
#include <LibC/string.h>

int toupper(int c) {
  if (c >= 'a' && c <= 'z')
    return c - ('a' - 'A');
  return c;
}

int tolower(int c) {
  if (c >= 'A' && c <= 'Z')
    return c + ('a' - 'A');
  return c;
}

void capitalize(char *str, size_t maxlen) {
  ASSERT(str != NULL);
  ASSERT(maxlen > 0);
  if (str[0] == '\0') return;

  str[0] = toupper(str[0]);

  for (size_t i = 1; i < maxlen && str[i] != '\0' && str[i] != ' '; i++)
    str[i] = tolower(str[i]);
}
