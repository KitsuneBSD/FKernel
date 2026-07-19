#pragma once

#ifdef __GLIBC__
#include <wchar.h>
#else

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t wchar_t;
typedef uint32_t wint_t;
typedef uint32_t wctype_t;

#define WEOF ((wint_t)-1)
#define WCHAR_MIN 0
#define WCHAR_MAX 0x10FFFF

// Wide string length
size_t wcslen(const wchar_t *s);

// Wide string copy
wchar_t *wcscpy(wchar_t *dst, const wchar_t *src);
wchar_t *wcsncpy(wchar_t *dst, const wchar_t *src, size_t n);

// Wide string compare
int wcscmp(const wchar_t *s1, const wchar_t *s2);
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n);

// Wide string concatenate
wchar_t *wcscat(wchar_t *dst, const wchar_t *src);
wchar_t *wcsncat(wchar_t *dst, const wchar_t *src, size_t n);

// Wide string search
wchar_t *wcschr(const wchar_t *s, wchar_t c);
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);

// Wide memory operations
wchar_t *wmemcpy(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);
int      wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n);

// Multibyte/wide character conversion
int    mbtowc(wchar_t *pwc, const char *s, size_t n);
int    wctomb(char *s, wchar_t wc);
size_t mbstowcs(wchar_t *dst, const char *src, size_t n);
size_t wcstombs(char *dst, const wchar_t *src, size_t n);
int    mblen(const char *s, size_t n);

#ifdef __cplusplus
}
#endif

#endif // __GLIBC__
