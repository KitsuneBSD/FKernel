#include "LibC/string.h"
#include <LibC/stdio.h>

namespace LibC {

static inline char* append_string(char* buf, char const* str)
{
    while (*str) {
        *buf++ = *str++;
    }
    return buf;
}

static inline char* append_char(char* buf, char c)
{
    *buf++ = c;
    return buf;
}

static inline char* append_padding(char* buf, char pad_char, int count)
{
    for (int i = 0; i < count; ++i)
        *buf++ = pad_char;
    return buf;
}

static inline char* format_integer(char* buf, LibC::int64_t val, int base, int width, char pad_char)
{
    char temp[65];
    LibC::size_t len = 0;

    bool negative = false;
    LibC::uint64_t uval;

    if (val < 0) {
        negative = true;
        uval = static_cast<LibC::uint64_t>(-val);
    } else {
        uval = static_cast<LibC::uint64_t>(val);
    }

    len = utoa(uval, temp, base);

    int const total_width = static_cast<int>(len) + (negative ? 1 : 0);
    if (total_width < width)
        buf = append_padding(buf, pad_char, width - total_width);

    if (negative)
        buf = append_char(buf, '-');

    for (size_t i = 0; i < len; ++i)
        *buf++ = temp[i];

    return buf;
}

static inline char* format_unsigned(char* buf, LibC::uint64_t val, int base, int width, char pad_char)
{
    char temp[65];
    LibC::size_t const len = utoa(val, temp, base);

    if (static_cast<int>(len) < width) {
        buf = append_padding(buf, pad_char, width - static_cast<int>(len));
    }

    for (LibC::size_t i = 0; i < len; ++i) {
        *buf++ = temp[i];
    }

    return buf;
}

static inline char* format_pointer(char* buf, void* ptr)
{
    buf = append_string(buf, "0x");
    return format_unsigned(buf, reinterpret_cast<uintptr_t>(ptr), 16, 0, '0');
}

int vsprintf(char* out, char const* fmt, va_list args)
{
    char* buf = out;

    while (*fmt) {
        if (*fmt != '%') {
            *buf++ = *fmt++;
            continue;
        }

        ++fmt;
        char pad_char = ' ';
        int width = 0;

        if (*fmt == '0') {
            pad_char = '0';
            ++fmt;
        }

        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            ++fmt;
        }

        bool long_flag = false;
        bool long_long_flag = false;

        if (*fmt == 'l') {
            ++fmt;
            if (*fmt == 'l') {
                ++fmt;
                long_long_flag = true;
            } else {
                long_flag = true;
            }
        }

        switch (*fmt) {
        case 'c':
            *buf++ = static_cast<char>(va_arg(args, int));
            break;

        case 's': {
            char const* str = va_arg(args, char const*);
            if (!str)
                str = "(null)";
            buf = append_string(buf, str);
            break;
        }

        case 'd':
        case 'i': {
            LibC::int64_t const val = long_long_flag ? va_arg(args, long long)
                                                     : (long_flag ? va_arg(args, long)
                                                                  : va_arg(args, int));
            buf = format_integer(buf, val, 10, width, pad_char);
            break;
        }

        case 'u': {
            LibC::uint64_t const val = long_long_flag ? va_arg(args, unsigned long long)
                                                      : (long_flag ? va_arg(args, unsigned long)
                                                                   : va_arg(args, unsigned int));
            buf = format_unsigned(buf, val, 10, width, pad_char);
            break;
        }

        case 'x':
        case 'X': {
            LibC::uint64_t const val = long_long_flag ? va_arg(args, unsigned long long)
                                                      : (long_flag ? va_arg(args, unsigned long)
                                                                   : va_arg(args, unsigned int));
            buf = format_unsigned(buf, val, 16, width, pad_char);
            break;
        }

        case 'p': {
            void* ptr = va_arg(args, void*);
            buf = format_pointer(buf, ptr);
            break;
        }

        case '%':
            *buf++ = '%';
            break;

        default:
            *buf++ = '%';
            if (*fmt)
                *buf++ = *fmt;
            break;
        }

        if (*fmt)
            ++fmt;
    }

    *buf = '\0';
    return static_cast<int>(buf - out);
}

int sprintf(char* out, char const* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int const written = vsprintf(out, fmt, args);
    va_end(args);
    return written;
}

}
