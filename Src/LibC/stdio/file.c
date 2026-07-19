#include <LibC/stdio.h>
#include <LibC/stdlib.h>
#include <LibC/stdarg.h>

// Static FILE objects for the standard streams
static FILE g_stdin_file  = { .fd = 0, .error_flag = 0, .eof_flag = 0 };
static FILE g_stdout_file = { .fd = 1, .error_flag = 0, .eof_flag = 0 };
static FILE g_stderr_file = { .fd = 2, .error_flag = 0, .eof_flag = 0 };

FILE* stdin  = &g_stdin_file;
FILE* stdout = &g_stdout_file;
FILE* stderr = &g_stderr_file;

// FILE operations — not supported in freestanding kernel LibC.
// User-space programs linked against a full C library (musl) should not use these.
FILE *fopen(const char *path, const char *mode) {
    (void)path; (void)mode;
    abort();
}

int fclose(FILE *stream) {
    (void)stream;
    abort();
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void)ptr; (void)size; (void)nmemb; (void)stream;
    abort();
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void)ptr; (void)size; (void)nmemb; (void)stream;
    abort();
}

int feof(FILE *stream) {
    if (!stream) return 1;
    return stream->eof_flag;
}

int ferror(FILE *stream) {
    if (!stream) return 1;
    return stream->error_flag;
}

void clearerr(FILE *stream) {
    if (!stream) return;
    stream->error_flag = 0;
    stream->eof_flag = 0;
}

int fflush(FILE *stream) {
    (void)stream;
    return 0;
}

long ftell(FILE *stream) {
    (void)stream;
    abort();
}

int fseek(FILE *stream, long offset, int whence) {
    (void)stream; (void)offset; (void)whence;
    abort();
}

int fgetc(FILE *stream) {
    (void)stream;
    return EOF;
}

int getchar(void) {
    return EOF;
}

char *fgets(char *s, int n, FILE *stream) {
    (void)s; (void)n; (void)stream;
    return (char *)0;
}

// sprintf / vsprintf - write to buffer (no size limit; unsafe but standard)
int vsprintf(char *str, const char *fmt, va_list args) {
    return vsnprintf(str, (size_t)-1, fmt, args);
}

int sprintf(char *str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsprintf(str, fmt, args);
    va_end(args);
    return n;
}

int sscanf(const char *str, const char *fmt, ...) {
    (void)str; (void)fmt;
    return EOF;
}
