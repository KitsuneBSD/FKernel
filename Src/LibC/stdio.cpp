#include <LibC/stdio.h>
#include <LibC/string.h>

namespace LibC {

static inline char* append_string(char* buf, char const* str)
{
    while (*str)
        *buf++ = *str++;
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

static inline char* format_integer(char* buf, int64_t val, int base, int width, char pad_char)
{
    char temp[65];
    size_t len = 0;

    bool negative = false;
    uint64_t uval;

    if (val < 0) {
        negative = true;
        uval = static_cast<uint64_t>(-val);
    } else {
        uval = static_cast<uint64_t>(val);
    }

    len = utoa(uval, temp, base, false);

    int total_width = static_cast<int>(len) + (negative ? 1 : 0);
    if (total_width < width)
        buf = append_padding(buf, pad_char, width - total_width);

    if (negative)
        buf = append_char(buf, '-');

    for (size_t i = 0; i < len; ++i)
        *buf++ = temp[i];

    return buf;
}

static inline char* format_unsigned(char* buf, uint64_t val, int base, int width, char pad_char, bool uppercase = false)
{
    char temp[65];
    size_t len = utoa(val, temp, base, uppercase);

    if (static_cast<int>(len) < width)
        buf = append_padding(buf, pad_char, width - static_cast<int>(len));

    for (size_t i = 0; i < len; ++i)
        *buf++ = temp[i];

    return buf;
}

static inline char* format_pointer(char* buf, void* ptr)
{
    buf = append_string(buf, "0x");
    return format_unsigned(buf, reinterpret_cast<uintptr_t>(ptr), 16, sizeof(uintptr_t) * 2, '0');
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
        bool size_t_flag = false;

        while (*fmt == 'l' || *fmt == 'z') {
            if (*fmt == 'l') {
                if (long_flag) {
                    long_long_flag = true;
                    long_flag = false;
                } else {
                    long_flag = true;
                }
                ++fmt;
            } else if (*fmt == 'z') {
                size_t_flag = true;
                ++fmt;
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
            int64_t val;
            if (long_long_flag)
                val = va_arg(args, long long);
            else if (long_flag)
                val = va_arg(args, long);
            else
                val = va_arg(args, int);
            buf = format_integer(buf, val, 10, width, pad_char);
            break;
        }

        case 'u':
        case 'x':
        case 'X': {
            uint64_t val;
            if (size_t_flag)
                val = va_arg(args, size_t);
            else if (long_long_flag)
                val = va_arg(args, unsigned long long);
            else if (long_flag)
                val = va_arg(args, unsigned long);
            else
                val = va_arg(args, unsigned int);

            int base = (*fmt == 'u') ? 10 : 16;
            bool uppercase = (*fmt == 'X');
            buf = format_unsigned(buf, val, base, width, pad_char, uppercase);
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
    int written = vsprintf(out, fmt, args);
    va_end(args);
    return written;
}

}
