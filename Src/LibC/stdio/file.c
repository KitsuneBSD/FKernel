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

