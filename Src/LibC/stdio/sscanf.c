#include <LibC/stdio.h>
#include <LibC/stdarg.h>

static int is_space(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}
static int is_digit(int c)  { return c >= '0' && c <= '9'; }
static int is_xdigit(int c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static int xdigit_val(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return c - 'A' + 10;
}

int vsscanf(const char *str, const char *fmt, va_list args) {
    const char *s = str;
    int matched = 0;

    while (*fmt) {
        if (is_space((unsigned char)*fmt)) {
            while (is_space((unsigned char)*s)) s++;
            fmt++;
            continue;
        }
        if (*fmt != '%') {
            if (*s != *fmt) break;
            s++; fmt++;
            continue;
        }
        fmt++; // consume '%'
        if (*fmt == '%') { if (*s++ != '%') break; fmt++; continue; }

        int width = 0;
        while (is_digit((unsigned char)*fmt)) width = width * 10 + (*fmt++ - '0');

        int lng = 0;
        if (*fmt == 'h') { lng = -1; fmt++; }
        else if (*fmt == 'l') { lng++; fmt++; if (*fmt == 'l') { lng++; fmt++; } }
        else if (*fmt == 'L') { lng = 2; fmt++; }

        char spec = *fmt++;
        if (!spec) break;

        if (spec != 'c' && spec != 'n' && spec != '[')
            while (is_space((unsigned char)*s)) s++;

        if (spec == 'n') {
            int *p = va_arg(args, int*);
            *p = (int)(s - str);
            continue;
        }
        if (!*s && spec != 'n') break;

        if (spec == 'c') {
            int n = width > 0 ? width : 1;
            char *p = va_arg(args, char*);
            while (n-- && *s) *p++ = *s++;
            matched++;
            continue;
        }
        if (spec == 's') {
            char *p = va_arg(args, char*);
            int n = 0;
            while (*s && !is_space((unsigned char)*s) && (width == 0 || n < width)) {
                *p++ = *s++; n++;
            }
            *p = '\0';
            if (n > 0) matched++;
            continue;
        }
        if (spec == 'd' || spec == 'i' || spec == 'u' ||
            spec == 'x' || spec == 'X' || spec == 'o') {
            const char *num_start = s;
            int neg = 0;
            if (spec == 'd' || spec == 'i') {
                if (*s == '-') { neg = 1; s++; }
                else if (*s == '+') s++;
            }
            const char *after_sign = s;
            int base = 10;
            if (spec == 'x' || spec == 'X') base = 16;
            else if (spec == 'o') base = 8;
            else if (spec == 'i') {
                if (*s == '0') {
                    s++;
                    if (*s == 'x' || *s == 'X') { base = 16; s++; }
                    else base = 8;
                }
            }
            unsigned long long val = 0;
            int n = 0;
            while ((width == 0 || n < width) && *s) {
                int d = -1;
                if (base == 16 && is_xdigit((unsigned char)*s)) d = xdigit_val(*s);
                else if (base == 8 && *s >= '0' && *s <= '7') d = *s - '0';
                else if (base == 10 && is_digit((unsigned char)*s)) d = *s - '0';
                if (d < 0) break;
                val = val * (unsigned long long)base + (unsigned long long)d;
                s++; n++;
            }
            if (n == 0) {
                // %i consumed "0" or "0x" prefix without digits: value is 0
                if (spec == 'i' && s > after_sign) { /* fall through: val=0 */ }
                else { s = num_start; break; }
            }
            long long sval = neg ? -(long long)val : (long long)val;
            if (spec == 'u' || spec == 'x' || spec == 'X' || spec == 'o') {
                if (lng == 2)       *va_arg(args, unsigned long long*) = val;
                else if (lng == 1)  *va_arg(args, unsigned long*)      = (unsigned long)val;
                else if (lng == -1) *va_arg(args, unsigned short*)     = (unsigned short)val;
                else                *va_arg(args, unsigned int*)        = (unsigned int)val;
            } else {
                if (lng == 2)       *va_arg(args, long long*) = sval;
                else if (lng == 1)  *va_arg(args, long*)      = (long)sval;
                else if (lng == -1) *va_arg(args, short*)     = (short)sval;
                else                *va_arg(args, int*)        = (int)sval;
            }
            matched++;
            continue;
        }
        break;
    }
    return matched > 0 ? matched : (s == str ? EOF : 0);
}

int sscanf(const char *str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsscanf(str, fmt, args);
    va_end(args);
    return n;
}
