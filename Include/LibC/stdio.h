#pragma once

#ifdef __GLIBC__

// Hosted/test environment: use system stdio for standard types and functions.
// Only declare kernel-specific hooks here.
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void kprintf(const char *fmt, ...);
void libc_puts(char *c);
void libc_set_heap_ready();
void libc_register_puts_hook(void (*fn)(const char *));

#ifdef __cplusplus
}
#endif

#else // !__GLIBC__ — freestanding kernel environment

#include <LibC/stdarg.h>
#include <LibC/stdbool.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

#ifndef __FILE_defined
#define __FILE_defined 1
typedef struct FILE_s {
    int    fd;
    int    error_flag;
    int    eof_flag;
    int    mode;        /* O_RDONLY / O_WRONLY / O_RDWR */
    int    is_heap;     /* 1 if allocated with malloc, 0 if static */
    size_t buf_pos;     /* next byte to read from buf */
    size_t buf_len;     /* valid bytes in buf */
    char   buf[BUFSIZ];
} FILE;
#endif

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

#ifndef EOF
#define EOF (-1)
#endif
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

// Internal hooks (set by kernel)
void libc_puts(char *c);
void libc_set_heap_ready();
void libc_register_puts_hook(void (*fn)(const char *));

int vsnprintf(char *str, size_t size, const char *fmt, va_list args);
int snprintf(char *str, size_t size, const char *fmt, ...);
void kprintf(const char *fmt, ...);

// Standard formatted output
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vprintf(const char *fmt, va_list args);
int fprintf(FILE *stream, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int vfprintf(FILE *stream, const char *fmt, va_list args);

// Character output
int putchar(int c);
int puts(const char *s);
int fputs(const char *s, FILE *stream);
int fputc(int c, FILE *stream);

// FILE operations
FILE *fopen(const char *path, const char *mode);
int   fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int   feof(FILE *stream);
int   ferror(FILE *stream);
void  clearerr(FILE *stream);
int   fflush(FILE *stream);
long  ftell(FILE *stream);
int   fseek(FILE *stream, long offset, int whence);
int   fgetc(FILE *stream);
int   getchar(void);
char *fgets(char *s, int n, FILE *stream);

int sprintf(char *str, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int vsprintf(char *str, const char *fmt, va_list args);
int sscanf(const char *str, const char *fmt, ...);
int vsscanf(const char *str, const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif

#endif // __GLIBC__
