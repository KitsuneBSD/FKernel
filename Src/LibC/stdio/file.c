#include <LibC/stdio.h>
#include <LibC/stdlib.h>
#include <LibC/stdarg.h>
#include <LibC/errno.h>
#include <LibC/fcntl.h>
#include <LibC/string.h>
#include <LibC/unistd.h>

static FILE g_stdin_file  = { .fd = 0, .mode = O_RDONLY };
static FILE g_stdout_file = { .fd = 1, .mode = O_WRONLY };
static FILE g_stderr_file = { .fd = 2, .mode = O_WRONLY };

FILE* stdin  = &g_stdin_file;
FILE* stdout = &g_stdout_file;
FILE* stderr = &g_stderr_file;

/* ---- fopen / fclose ------------------------------------------------------ */

static int parse_mode(const char *mode) {
    if (!mode) return -1;
    if (mode[0] == 'r') return (mode[1] == '+') ? O_RDWR : O_RDONLY;
    if (mode[0] == 'w') return (mode[1] == '+') ? (O_RDWR|O_CREAT|O_TRUNC) : (O_WRONLY|O_CREAT|O_TRUNC);
    if (mode[0] == 'a') return (mode[1] == '+') ? (O_RDWR|O_CREAT|O_APPEND) : (O_WRONLY|O_CREAT|O_APPEND);
    return -1;
}

FILE *fopen(const char *path, const char *mode) {
    int flags = parse_mode(mode);
    if (flags < 0) { errno = EINVAL; return NULL; }

    int fd = open(path, flags, 0666);
    if (fd < 0) return NULL;

    FILE *f = (FILE*)malloc(sizeof(FILE));
    if (!f) { close(fd); errno = ENOMEM; return NULL; }

    f->fd         = fd;
    f->error_flag = 0;
    f->eof_flag   = 0;
    f->mode       = flags & O_ACCMODE;
    f->is_heap    = 1;
    f->buf_pos    = 0;
    f->buf_len    = 0;
    return f;
}

int fclose(FILE *stream) {
    if (!stream) { errno = EBADF; return EOF; }
    fflush(stream);
    int ret = close(stream->fd);
    if (stream->is_heap) free(stream);
    return ret < 0 ? EOF : 0;
}

int fflush(FILE *stream) {
    (void)stream;
    return 0;
}

/* ---- read buffer --------------------------------------------------------- */

static int fill_read_buf(FILE *stream) {
    stream->buf_pos = 0;
    stream->buf_len = 0;
    ssize_t n = read(stream->fd, stream->buf, BUFSIZ);
    if (n < 0) { stream->error_flag = 1; return -1; }
    if (n == 0) { stream->eof_flag  = 1; return 0; }
    stream->buf_len = (size_t)n;
    return (int)n;
}

/* ---- fread / fwrite ------------------------------------------------------ */

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (!stream || stream->error_flag) return 0;
    if (size == 0 || nmemb == 0) return 0;

    size_t total  = size * nmemb;
    size_t copied = 0;
    char *dst     = (char*)ptr;

    while (copied < total) {
        if (stream->buf_pos >= stream->buf_len) {
            if (fill_read_buf(stream) <= 0) break;
        }
        size_t avail = stream->buf_len - stream->buf_pos;
        size_t need  = total - copied;
        size_t take  = avail < need ? avail : need;
        memcpy(dst + copied, stream->buf + stream->buf_pos, take);
        stream->buf_pos += take;
        copied          += take;
    }
    return (size > 0) ? (copied / size) : 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (!stream || stream->error_flag) return 0;
    if (size == 0 || nmemb == 0) return 0;

    size_t  total = size * nmemb;
    ssize_t n     = write(stream->fd, ptr, total);
    if (n < 0) { stream->error_flag = 1; return 0; }
    return (size > 0) ? ((size_t)n / size) : 0;
}

/* ---- character I/O ------------------------------------------------------- */

int fgetc(FILE *stream) {
    if (!stream || stream->error_flag || stream->eof_flag) return EOF;
    if (stream->buf_pos >= stream->buf_len) {
        if (fill_read_buf(stream) <= 0) return EOF;
    }
    return (unsigned char)stream->buf[stream->buf_pos++];
}

int getchar(void) { return fgetc(stdin); }

/* ---- fgets --------------------------------------------------------------- */

char *fgets(char *s, int n, FILE *stream) {
    if (!s || n <= 0 || !stream) return NULL;
    int i = 0;
    while (i < n - 1) {
        int c = fgetc(stream);
        if (c == EOF) { if (i == 0) return NULL; break; }
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    s[i] = '\0';
    return s;
}

/* ---- fseek / ftell ------------------------------------------------------- */

int fseek(FILE *stream, long offset, int whence) {
    if (!stream) { errno = EBADF; return -1; }
    stream->buf_pos = 0;
    stream->buf_len = 0;
    off_t r = lseek(stream->fd, (off_t)offset, whence);
    if (r < 0) { stream->error_flag = 1; return -1; }
    return 0;
}

long ftell(FILE *stream) {
    if (!stream) { errno = EBADF; return -1L; }
    long buf_unread = (long)(stream->buf_len - stream->buf_pos);
    off_t pos = lseek(stream->fd, 0, SEEK_CUR);
    if (pos < 0) { stream->error_flag = 1; return -1L; }
    return (long)pos - buf_unread;
}

/* ---- error / eof --------------------------------------------------------- */

int feof(FILE *stream)   { return !stream || stream->eof_flag; }
int ferror(FILE *stream) { return !stream || stream->error_flag; }

void clearerr(FILE *stream) {
    if (!stream) return;
    stream->error_flag = 0;
    stream->eof_flag   = 0;
}

/* ---- sprintf / vsprintf -------------------------------------------------- */

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

/* ---- sscanf / vsscanf ---------------------------------------------------- */

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
            int neg = 0;
            if (spec == 'd' || spec == 'i') {
                if (*s == '-') { neg = 1; s++; }
                else if (*s == '+') s++;
            }
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
            if (n == 0 && spec != 'i') break;
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
