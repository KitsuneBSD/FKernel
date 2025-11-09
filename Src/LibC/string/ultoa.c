#include <LibC/string.h>

size_t ultoa(unsigned long value, char *buffer, unsigned int base) {
  if (base < 2 || base > 16) {
    buffer[0] = '\0';
    return 0;
  }

  char temp[32];
  size_t i = 0;

  if (value == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
    return 1;
  }

  while (value > 0) {
    unsigned long digit = value % base;
    if (digit < 10)
      temp[i++] = '0' + digit;
    else
      temp[i++] = 'a' + (digit - 10);
    value /= base;
  }

  size_t j = 0;
  while (i > 0) {
    buffer[j++] = temp[--i];
  }
  buffer[j] = '\0';
  return j;
}
