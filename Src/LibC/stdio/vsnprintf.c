#include <LibC/stdarg.h>
#include <LibC/stdbool.h>
#include <LibC/stdint.h>
#include <LibC/stdio.h>
#include <LibC/string.h>

static void write_char(char *buf, size_t *idx, size_t max, char c) {
    if (*idx < max - 1) buf[(*idx)++] = c;
}

static void write_padding(char *buf, size_t *idx, size_t max, int count, char pad_char) {
    while (count-- > 0) write_char(buf, idx, max, pad_char);
}

static void print_num(char *buf, size_t *idx, size_t max, uint64_t uval, bool negative,
                      int base, bool uppercase, int width, bool zero_pad, bool left_align, bool always_sign, bool alt_form) {
    char tmp[66];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;

    uint64_t val_copy = uval;
    if (val_copy == 0) tmp[i++] = '0';
    else while (val_copy > 0) { tmp[i++] = digits[val_copy % base]; val_copy /= base; }

    char prefix[3];
    int prefix_len = 0;
    if (negative) prefix[prefix_len++] = '-';
    else if (always_sign) prefix[prefix_len++] = '+';
    
    if (alt_form && (uval != 0 || base == 16)) { // Always add 0x for pointers (alt_form is set for %p)
        if (base == 16) {
            prefix[prefix_len++] = '0';
            prefix[prefix_len++] = uppercase ? 'X' : 'x';
        } else if (base == 8 && tmp[i-1] != '0') {
            prefix[prefix_len++] = '0';
        }
    }

    int padding = width - (i + prefix_len);
    if (padding < 0) padding = 0;

    if (!left_align && !zero_pad) write_padding(buf, idx, max, padding, ' ');
    for (int j = 0; j < prefix_len; j++) write_char(buf, idx, max, prefix[j]);
    if (!left_align && zero_pad) write_padding(buf, idx, max, padding, '0');
    while (i > 0) write_char(buf, idx, max, tmp[--i]);
    if (left_align) write_padding(buf, idx, max, padding, ' ');
}

int vsnprintf(char *buf, size_t max, const char *fmt, va_list args) {
    if (!buf || max == 0 || !fmt) return 0;
    size_t idx = 0;
    const char *p = fmt;
    while (*p && idx < max - 1) {
        if (*p != '%') { write_char(buf, &idx, max, *p++); continue; }
        p++; // Skip '%'
        
        bool alt_form = false, zero_pad = false, left_align = false, always_sign = false;
        while (*p) {
            if (*p == '#') alt_form = true;
            else if (*p == '0') zero_pad = true;
            else if (*p == '-') left_align = true;
            else if (*p == '+') always_sign = true;
            else break;
            p++;
        }

        int width = 0;
        if (*p == '*') { width = va_arg(args, int); p++; }
        else while (*p >= '0' && *p <= '9') width = width * 10 + (*p++ - '0');

        int precision = -1;
        if (*p == '.') {
            p++;
            if (*p == '*') { precision = va_arg(args, int); p++; }
            else { precision = 0; while (*p >= '0' && *p <= '9') precision = precision * 10 + (*p++ - '0'); }
        }

        bool long_flag = false, long_long_flag = false, size_t_flag = false;
        while (*p == 'l' || *p == 'z' || *p == 'h') {
            if (*p == 'l') { p++; if (*p == 'l') { long_long_flag = true; p++; } else long_flag = true; }
            else if (*p == 'z') { size_t_flag = true; p++; }
            else if (*p == 'h') { p++; if (*p == 'h') p++; }
        }
        if (!*p) break;

        switch (*p) {
            case 'c': write_char(buf, &idx, max, (char)va_arg(args, int)); break;
            case 's': {
                const char *s = va_arg(args, const char *);
                if (!s) s = "(null)";
                size_t len = strlen(s);
                if (precision >= 0 && (size_t)precision < len) len = precision;
                int padding = width - (int)len;
                if (padding < 0) padding = 0;
                if (!left_align) write_padding(buf, &idx, max, padding, ' ');
                for (size_t j = 0; j < len && idx < max - 1; j++) write_char(buf, &idx, max, s[j]);
                if (left_align) write_padding(buf, &idx, max, padding, ' ');
                break;
            }
            case 'd':
            case 'i': {
                int64_t val;
                if (long_long_flag) val = va_arg(args, long long);
                else if (long_flag) val = va_arg(args, long);
                else val = (int64_t)va_arg(args, int);
                print_num(buf, &idx, max, (uint64_t)(val < 0 ? -val : val), val < 0, 10, false, width, zero_pad, left_align, always_sign, false);
                break;
            }
            case 'u':
            case 'o':
            case 'x':
            case 'X': {
                uint64_t val;
                if (long_long_flag) val = va_arg(args, unsigned long long);
                else if (long_flag) val = va_arg(args, unsigned long);
                else if (size_t_flag) val = (uint64_t)va_arg(args, size_t);
                else val = (uint64_t)va_arg(args, unsigned int);
                int base = (*p == 'o') ? 8 : ((*p == 'u') ? 10 : 16);
                print_num(buf, &idx, max, val, false, base, (*p == 'X'), width, zero_pad, left_align, always_sign, alt_form);
                break;
            }
            case 'p': {
                print_num(buf, &idx, max, (uintptr_t)va_arg(args, void *), false, 16, false, width ? width : 18, true, left_align, false, true);
                break;
            }
            case '%': write_char(buf, &idx, max, '%'); break;
            default: write_char(buf, &idx, max, *p); break;
        }
        p++;
    }
    buf[idx] = '\0';
    return (int)idx;
}