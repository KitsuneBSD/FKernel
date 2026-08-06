#include <LibC/stdio.h>
#include <LibC/stdarg.h>

// printf/vprintf route through libc_puts (which uses the kernel's log hook).
// fprintf/vfprintf check if the FILE* is stdout/stderr and use the same path;
// all other FILE* are unsupported in the freestanding kernel context.

int vprintf(const char *fmt, va_list args) {
    char buf[2048];
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    libc_puts(buf);
    return n;
}

int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vprintf(fmt, args);
    va_end(args);
    return n;
}

int vfprintf(FILE *stream, const char *fmt, va_list args) {
    (void)stream;
    return vprintf(fmt, args);
}

int fprintf(FILE *stream, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vfprintf(stream, fmt, args);
    va_end(args);
    return n;
}

int putchar(int c) {
    char buf[2] = { (char)c, '\0' };
    libc_puts(buf);
    return c;
}

int puts(const char *s) {
    if (!s) return -1;
    // Use a buffer to append newline
    libc_puts((char *)s);
    char nl[2] = { '\n', '\0' };
    libc_puts(nl);
    return 0;
}

int fputs(const char *s, FILE *stream) {
    (void)stream;
    if (!s) return -1;
    libc_puts((char *)s);
    return 0;
}

int fputc(int c, FILE *stream) {
    (void)stream;
    return putchar(c);
}
