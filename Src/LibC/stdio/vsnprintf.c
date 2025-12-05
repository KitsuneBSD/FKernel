#include <LibC/assert.h>
#include <LibC/stdarg.h>
#include <LibC/stdbool.h>
#include <LibC/stdint.h>
#include <LibC/stdio.h>
#include <LibC/string.h>

static void write_padding(char *buf, size_t *idx, size_t max, int count,
                          char pad_char) {
  ASSERT(buf != NULL); // Assert buffer is not null
  ASSERT(idx != NULL); // Assert index pointer is not null
  ASSERT(max > 0);     // Assert max size is positive
  ASSERT(count >= 0);  // Assert padding count is non-negative

  while (count-- > 0 && *idx < max - 1) {
    buf[(*idx)++] = pad_char;
  }
}

static void print_uint(char *buf, size_t *idx, size_t max, uint64_t value,
                       int base, bool uppercase, int width, bool zero_pad) {
  ASSERT(buf != NULL);             // Assert buffer is not null
  ASSERT(idx != NULL);             // Assert index pointer is not null
  ASSERT(max > 0);                 // Assert max size is positive
  ASSERT(base >= 2 && base <= 16); // Base must be between 2 and 16
  ASSERT(width >= 0);              // Width cannot be negative
  ASSERT(sizeof(char[65]) > 0);    // Ensure tmp buffer is valid

  char tmp[65];
  const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
  int i = 0;

  if (value == 0) {
    tmp[i++] = '0';
  } else {
    while (value > 0 && i < (int)sizeof(tmp) - 1) {
      tmp[i++] = digits[value % base];
      value /= base;
    }
  }

  int padding = width - i;
  if (padding < 0)
    padding = 0;

  if (!zero_pad) {
    write_padding(buf, idx, max, padding, ' ');
  } else {
    write_padding(buf, idx, max, padding, '0');
  }

  for (int j = i - 1; j >= 0 && *idx < max - 1; j--)
    buf[(*idx)++] = tmp[j];
}

static void print_int(char *buf, size_t *idx, size_t max, int64_t value,
                      int width, bool zero_pad, bool always_sign) {
  ASSERT(buf != NULL); // Assert buffer is not null
  ASSERT(idx != NULL); // Assert index pointer is not null
  ASSERT(max > 0);     // Assert max size is positive
  ASSERT(width >= 0);  // Width cannot be negative

  if (value < 0) {
    if (*idx < max - 1)
      buf[(*idx)++] = '-';
    print_uint(buf, idx, max, (uint64_t)(-value), 10, false, width, zero_pad);
  } else {
    if (always_sign) {
      if (*idx < max - 1)
        buf[(*idx)++] = '+';
    }
    print_uint(buf, idx, max, (uint64_t)value, 10, false, width, zero_pad);
  }
}

// New helper: Parses format flags
static void _parse_flags(const char *fmt, size_t *i, bool *left_align,
                         bool *zero_pad, bool *always_sign, bool *alt_form) {
  ASSERT(fmt != NULL);
  ASSERT(i != NULL);
  ASSERT(left_align != NULL);
  ASSERT(zero_pad != NULL);
  ASSERT(always_sign != NULL);
  ASSERT(alt_form != NULL);
  while (true) {
    switch (fmt[*i]) {
    case '-':
      *left_align = true;
      (*i)++;
      continue;
    case '0':
      *zero_pad = true;
      (*i)++;
      continue;
    case '+':
      *always_sign = true;
      (*i)++;
      continue;
    case '#':
      *alt_form = true;
      (*i)++;
      continue;
    }
    break;
  }
}

// New helper: Parses width
static int _parse_width(const char *fmt, size_t *i) {
  ASSERT(fmt != NULL);
  ASSERT(i != NULL);
  int width = 0;
  while (fmt[*i] >= '0' && fmt[*i] <= '9') {
    width = width * 10 + (fmt[(*i)++] - '0');
  }
  return width;
}

// New helper: Parses length modifiers
static void _parse_length_modifier(const char *fmt, size_t *i, bool *long_flag,
                                   bool *size_t_flag) {
  ASSERT(fmt != NULL);
  ASSERT(i != NULL);
  ASSERT(long_flag != NULL);
  ASSERT(size_t_flag != NULL);
  if (fmt[*i] == 'l') {
    *long_flag = true;
    (*i)++;
  } else if (fmt[*i] == 'z') {
    *size_t_flag = true;
    (*i)++;
  }
}

// New helper: Handles %c
static void _handle_char_specifier(char *buf, size_t max, size_t *idx,
                                   va_list args) {
  ASSERT(buf != NULL);
  ASSERT(idx != NULL);
  ASSERT(max > 0);
  char c = (char)va_arg(args, int);
  if (*idx < max - 1)
    buf[(*idx)++] = c;
}

// New helper: Handles %s
static void _handle_string_specifier(char *buf, size_t max, size_t *idx,
                                     va_list args) {
  ASSERT(buf != NULL);
  ASSERT(idx != NULL);
  ASSERT(max > 0);
  const char *str_val = va_arg(args, const char *);
  ASSERT(str_val != NULL);
  size_t len = strnlen(str_val, max - *idx - 1);
  for (size_t j = 0; j < len && *idx < max - 1; j++)
    buf[(*idx)++] = str_val[j];
}

// New helper: Handles %d, %i
static void _handle_int_specifier(char *buf, size_t max, size_t *idx,
                                  va_list args, bool long_flag, int width,
                                  bool zero_pad, bool always_sign) {
  ASSERT(buf != NULL);
  ASSERT(idx != NULL);
  ASSERT(max > 0);
  if (long_flag)
    print_int(buf, idx, max, va_arg(args, long), width, zero_pad, always_sign);
  else
    print_int(buf, idx, max, va_arg(args, int), width, zero_pad, always_sign);
}

// New helper: Handles %u
static void _handle_uint_specifier(char *buf, size_t max, size_t *idx,
                                   va_list args, bool long_flag,
                                   bool size_t_flag, int width, bool zero_pad) {
  ASSERT(buf != NULL);
  ASSERT(idx != NULL);
  ASSERT(max > 0);
  if (long_flag)
    print_uint(buf, idx, max, va_arg(args, unsigned long), 10, false, width,
               zero_pad);
  else if (size_t_flag)
    print_uint(buf, idx, max, va_arg(args, size_t), 10, false, width, zero_pad);
  else
    print_uint(buf, idx, max, va_arg(args, unsigned int), 10, false, width,
               zero_pad);
}

// New helper: Handles %x, %X
static void _handle_hex_specifier(char *buf, size_t max, size_t *idx,
                                  va_list args, bool long_flag,
                                  bool size_t_flag, int width, bool zero_pad,
                                  bool uppercase, bool alt_form) {
  ASSERT(buf != NULL);
  ASSERT(idx != NULL);
  ASSERT(max > 0);
  if (alt_form && *idx + 2 < max - 1) {
    buf[(*idx)++] = '0';
    buf[(*idx)++] = (uppercase ? 'X' : 'x');
  }
  if (long_flag)
    print_uint(buf, idx, max, va_arg(args, unsigned long), 16, uppercase, width,
               zero_pad);
  else if (size_t_flag)
    print_uint(buf, idx, max, va_arg(args, size_t), 16, uppercase, width,
               zero_pad);
  else
    print_uint(buf, idx, max, va_arg(args, unsigned int), 16, uppercase, width,
               zero_pad);
}

// New helper: Handles %p
static void _handle_pointer_specifier(char *buf, size_t max, size_t *idx,
                                      va_list args) {
  ASSERT(buf != NULL);
  ASSERT(idx != NULL);
  ASSERT(max > 0);
  void *ptr = va_arg(args, void *);
  uintptr_t val = (uintptr_t)ptr;
  if (*idx + 2 < max - 1) {
    buf[(*idx)++] = '0';
    buf[(*idx)++] = 'x';
  }
  // Pointers are typically zero-padded to the width of the pointer type.
  // For a 64-bit system, this means 16 hexadecimal digits (sizeof(uintptr_t) *
  // 2). For a 32-bit system, this means 8 hexadecimal digits.
  print_uint(buf, idx, max, val, 16, false, sizeof(uintptr_t) * 2, true);
}

int vsnprintf(char *buf, size_t max, const char *fmt, va_list args) {
  ASSERT(buf != NULL);
  ASSERT(fmt != NULL);
  ASSERT(max > 0);
  ASSERT(strlen(fmt) < 256);

  size_t idx = 0;
  size_t i = 0;
  size_t fmt_len = strlen(fmt);

  while (i < fmt_len && idx < max - 1) {
    if (fmt[i] != '%') {
      buf[idx++] = fmt[i];
      i++;
      continue;
    }

    i++;
    if (i >= fmt_len)
      break;

    bool long_flag = false;
    bool size_t_flag = false;
    bool left_align = false;
    bool zero_pad = false;
    bool always_sign = false;
    bool alt_form = false;

    _parse_flags(fmt, &i, &left_align, &zero_pad, &always_sign, &alt_form);
    int width = _parse_width(fmt, &i);
    _parse_length_modifier(fmt, &i, &long_flag, &size_t_flag);

    switch (fmt[i]) {
    case 'c':
      _handle_char_specifier(buf, max, &idx, args);
      break;
    case 's':
      _handle_string_specifier(buf, max, &idx, args);
      break;
    case 'd':
    case 'i':
      _handle_int_specifier(buf, max, &idx, args, long_flag, width, zero_pad,
                            always_sign);
      break;
    case 'u':
      _handle_uint_specifier(buf, max, &idx, args, long_flag, size_t_flag,
                             width, zero_pad);
      break;
    case 'x':
      _handle_hex_specifier(buf, max, &idx, args, long_flag, size_t_flag, width,
                            zero_pad, false, alt_form);
      break;
    case 'X':
      _handle_hex_specifier(buf, max, &idx, args, long_flag, size_t_flag, width,
                            zero_pad, true, alt_form);
      break;
    case 'p':
      _handle_pointer_specifier(buf, max, &idx, args);
      break;
    case '%':
      if (idx < max - 1)
        buf[idx++] = '%';
      break;
    default:
      if (idx < max - 1)
        buf[idx++] = fmt[i];
      break;
    }
    i++;
  }

  buf[idx] = '\0';
  return (int)idx;
}
