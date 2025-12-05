#include <LibC/assert.h>
#include <LibC/stdarg.h>
#include <LibC/stdbool.h>
#include <LibC/stdio.h>
#include <LibC/string.h>


static void write_int_to_buffer(char *str, size_t size, size_t *pos, int val, int base) {
  char num_buf[32];
  ASSERT(sizeof(num_buf) >= 32); // Ensure num_buf is large enough
  ASSERT(base >= 2 && base <= 16); // Assert valid base
  int len = itoa(val, num_buf, base);
  for (int j = 0; j < len && *pos < size - 1; j++) {
    str[(*pos)++] = num_buf[j];
  }
}

static void write_uint_to_buffer(char *str, size_t size, size_t *pos, unsigned int val, int base, bool uppercase) {
  char num_buf[32];
  ASSERT(sizeof(num_buf) >= 32); // Ensure num_buf is large enough
  ASSERT(base >= 2 && base <= 16); // Assert valid base
  itoa_unsigned(val, num_buf, base, uppercase);
  int len = strlen(num_buf);
  for (int j = 0; j < len && *pos < size - 1; j++) {
    str[(*pos)++] = num_buf[j];
  }
}

static void write_string_to_buffer(char *str, size_t size, size_t *pos, const char *s_val) {
  ASSERT(s_val != NULL); // Assert string pointer is not null
  size_t s_len = strnlen(s_val, size - *pos - 1);
  for (size_t j = 0; j < s_len; j++) {
    str[(*pos)++] = s_val[j];
  }
}

int snprintf(char *str, size_t size, const char *fmt, ...) {
  ASSERT(str != NULL);
  ASSERT(fmt != NULL);
  ASSERT(size > 0);
  ASSERT(strlen(fmt) < 256); // Limit fmt string length to prevent unbounded loops

  va_list args;
  va_start(args, fmt);

  size_t i = 0;
  size_t pos = 0;
  size_t fmt_len = strlen(fmt); // Cache format string length for fixed bound

  while (i < fmt_len && pos < size - 1) {
    if (fmt[i] == '%') {
      i++;
      if (fmt[i] == 'd') {
        int val = va_arg(args, int);
        write_int_to_buffer(str, size, &pos, val, 10);
      } else if (fmt[i] == 'x') {
        int val = va_arg(args, int);
        write_int_to_buffer(str, size, &pos, val, 16);
      } else if (fmt[i] == 'u') {
        unsigned int val = va_arg(args, unsigned int);
        write_uint_to_buffer(str, size, &pos, val, 10, false);
      } else if (fmt[i] == 's') {
        char *s = va_arg(args, char *);
        write_string_to_buffer(str, size, &pos, s);
      } else {
        str[pos++] = '%';
        if (pos < size - 1)
          str[pos++] = fmt[i];
      }
    } else {
      str[pos++] = fmt[i];
    }
    i++;
  }

  str[pos] = '\0';
  va_end(args);
  return pos;
}
