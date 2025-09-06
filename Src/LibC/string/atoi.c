#include <LibC/string.h>

int atoi(const char *str) {
  int result = 0;
  int sign = 1;

  while (*str == ' ' || *str == '\t' || *str == '\n')
    str++;

  if (*str == '-') {
    sign = -1;
    str++;
  } else if (*str == '+') {
    str++;
  }

  while (*str >= '0' && *str <= '9') {
    result = result * 10 + (*str - '0');
    str++;
  }

  return sign * result;
}
