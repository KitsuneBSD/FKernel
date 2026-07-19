#include <LibC/wchar.h>
#include <LibC/string.h>

size_t wcslen(const wchar_t *s) {
  size_t n = 0;
  while (s[n]) ++n;
  return n;
}

wchar_t *wcscpy(wchar_t *dst, const wchar_t *src) {
  wchar_t *d = dst;
  while ((*d++ = *src++));
  return dst;
}

wchar_t *wcsncpy(wchar_t *dst, const wchar_t *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i]; ++i) dst[i] = src[i];
  for (; i < n; ++i) dst[i] = 0;
  return dst;
}

int wcscmp(const wchar_t *s1, const wchar_t *s2) {
  while (*s1 && *s1 == *s2) { ++s1; ++s2; }
  return (int)(*s1 - *s2);
}

int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n) {
  while (n-- && *s1 && *s1 == *s2) { ++s1; ++s2; }
  if (n == (size_t)-1) return 0;
  return (int)(*s1 - *s2);
}

wchar_t *wcscat(wchar_t *dst, const wchar_t *src) {
  wchar_t *d = dst + wcslen(dst);
  while ((*d++ = *src++));
  return dst;
}

wchar_t *wcsncat(wchar_t *dst, const wchar_t *src, size_t n) {
  wchar_t *d = dst + wcslen(dst);
  size_t i = 0;
  for (; i < n && src[i]; ++i) d[i] = src[i];
  d[i] = 0;
  return dst;
}

wchar_t *wcschr(const wchar_t *s, wchar_t c) {
  while (*s && *s != c) ++s;
  return (*s == c) ? (wchar_t *)s : (void*)0;
}

wchar_t *wcsrchr(const wchar_t *s, wchar_t c) {
  const wchar_t *last = (void*)0;
  while (*s) { if (*s == c) last = s; ++s; }
  if (c == 0) return (wchar_t *)s;
  return (wchar_t *)last;
}

wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle) {
  size_t nlen = wcslen(needle);
  if (nlen == 0) return (wchar_t *)haystack;
  for (; *haystack; ++haystack) {
    if (wcsncmp(haystack, needle, nlen) == 0)
      return (wchar_t *)haystack;
  }
  return (void*)0;
}

wchar_t *wmemcpy(wchar_t *dst, const wchar_t *src, size_t n) {
  for (size_t i = 0; i < n; ++i) dst[i] = src[i];
  return dst;
}

wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n) {
  for (size_t i = 0; i < n; ++i) s[i] = c;
  return s;
}

int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n) {
  for (size_t i = 0; i < n; ++i)
    if (s1[i] != s2[i]) return (int)(s1[i] - s2[i]);
  return 0;
}

/* Minimal ASCII-only multibyte/wide conversions */
int mbtowc(wchar_t *pwc, const char *s, size_t n) {
  if (!s) return 0;
  if (n == 0) return -1;
  if (pwc) *pwc = (wchar_t)(unsigned char)*s;
  return (*s == '\0') ? 0 : 1;
}

int wctomb(char *s, wchar_t wc) {
  if (!s) return 0;
  if (wc < 0x80) { *s = (char)wc; return 1; }
  return -1;
}

size_t mbstowcs(wchar_t *dst, const char *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i]; ++i) {
    if (dst) dst[i] = (wchar_t)(unsigned char)src[i];
  }
  if (i < n && dst) dst[i] = 0;
  return i;
}

size_t wcstombs(char *dst, const wchar_t *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i]; ++i) {
    if (src[i] >= 0x80) return (size_t)-1;
    if (dst) dst[i] = (char)src[i];
  }
  if (i < n && dst) dst[i] = '\0';
  return i;
}

int mblen(const char *s, size_t n) {
  if (!s) return 0;
  if (n == 0) return -1;
  return (*s == '\0') ? 0 : 1;
}
