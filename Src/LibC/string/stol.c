#include <LibC/assert.h>
#include <LibC/string.h>

long stol(const char *str) {
  ASSERT(str != NULL);

  long result = 0;
  int sign = 1;
  size_t count = 0;

  while ((*str == ' ' || *str == '\t' || *str == '\n') &&
         count < MAX_ATOI_LEN) {
    str++;
    count++;
  }

  if (count == MAX_ATOI_LEN)
    return 0; // Or handle error appropriately

  count = 0;
  if (*str == '-') {
    sign = -1;
    str++;
  } else if (*str == '+') {
    str++;
  }
  ASSERT(*str >= '0' && *str <= '9'); // Ensure there's at least one digit

  while ((*str >= '0' && *str <= '9') && count < MAX_ATOI_LEN) {
    result = result * 10 + (*str - '0');
    str++;
    count++;
  }

  return sign * result;
}
