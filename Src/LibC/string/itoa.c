#include <LibC/string.h>

int itoa(int val, char *buf, int base) {
  char tmp[32];
  int i = 0, j = 0;
  int is_negative = 0;

  if (val == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return 1;
  }

  if (base == 10 && val < 0) {
    is_negative = 1;
    val = -val;
  }

  while (val && i < 31) {
    tmp[i++] = digits[val % base];
    val /= base;
  }

  if (is_negative) {
    tmp[i++] = '-';
  }

  while (i > 0) {
    buf[j++] = tmp[--i];
  }

  buf[j] = '\0';
  return j;
}
