#include <LibC/stdlib.h>
#include <LibC/string.h>

/* Provided by LibFK/heap_malloc.cpp */
extern void *kmalloc(size_t size);
extern void  kfree(void *ptr);
extern void *kcalloc(size_t nmemb, size_t size);
extern void *krealloc(void *ptr, size_t size);

void *malloc(size_t size)               { return kmalloc(size); }
void  free(void *ptr)                   { kfree(ptr); }
void *calloc(size_t nmemb, size_t size) { return kcalloc(nmemb, size); }
void *realloc(void *ptr, size_t size)   { return krealloc(ptr, size); }

/* abort/exit - implemented in Kernel/Arch/x86_64/Panic/Panic.cpp */
extern void __kernel_assert_fail(const char *expr, const char *file, int line, const char *func);

void abort(void) {
  __kernel_assert_fail("abort()", __FILE__, __LINE__, __func__);
  __builtin_unreachable();
}

void exit(int status) {
  (void)status;
  abort();
}

/* atol */
long atol(const char *str) {
  long result = 0;
  int sign = 1;
  while (*str == ' ' || *str == '\t') ++str;
  if (*str == '-') { sign = -1; ++str; }
  else if (*str == '+') ++str;
  while (*str >= '0' && *str <= '9')
    result = result * 10 + (*str++ - '0');
  return sign * result;
}

/* strtol */
long strtol(const char *str, char **endptr, int base) {
  long result = 0;
  int sign = 1;
  while (*str == ' ' || *str == '\t') ++str;
  if (*str == '-') { sign = -1; ++str; }
  else if (*str == '+') ++str;
  if (base == 0) {
    if (*str == '0') { ++str; base = (*str == 'x' || *str == 'X') ? (++str, 16) : 8; }
    else base = 10;
  } else if (base == 16 && *str == '0' && (str[1] == 'x' || str[1] == 'X')) str += 2;
  const char *start = str;
  while (1) {
    int digit;
    if (*str >= '0' && *str <= '9') digit = *str - '0';
    else if (*str >= 'a' && *str <= 'z') digit = *str - 'a' + 10;
    else if (*str >= 'A' && *str <= 'Z') digit = *str - 'A' + 10;
    else break;
    if (digit >= base) break;
    result = result * base + digit;
    ++str;
  }
  if (endptr) *endptr = (str == start) ? (char *)str : (char *)str;
  return sign * result;
}

/* strtoul */
unsigned long strtoul(const char *str, char **endptr, int base) {
  return (unsigned long)strtol(str, endptr, base);
}

/* qsort - simple insertion sort for now */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
  char *arr = (char *)base;
  char tmp[512];
  if (size > sizeof(tmp)) return;
  for (size_t i = 1; i < nmemb; ++i) {
    memcpy(tmp, arr + i * size, size);
    size_t j = i;
    while (j > 0 && compar(arr + (j - 1) * size, tmp) > 0) {
      memcpy(arr + j * size, arr + (j - 1) * size, size);
      --j;
    }
    memcpy(arr + j * size, tmp, size);
  }
}

/* bsearch */
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *)) {
  const char *lo = (const char *)base;
  const char *hi = lo + nmemb * size;
  while (lo < hi) {
    const char *mid = lo + ((hi - lo) / size / 2) * size;
    int cmp = compar(key, mid);
    if (cmp == 0) return (void *)mid;
    if (cmp < 0) hi = mid;
    else lo = mid + size;
  }
  return NULL;
}

/* strtoll */
long long strtoll(const char *str, char **endptr, int base) {
  return (long long)strtol(str, endptr, base);
}

/* strtoull */
unsigned long long strtoull(const char *str, char **endptr, int base) {
  return (unsigned long long)strtol(str, endptr, base);
}

/* strtod / strtof — not available in freestanding kernel (no SSE for float returns).
   Only compiled when SSE is enabled (userspace LibC context). */
#ifndef __FKERNEL_FREESTANDING__
double strtod(const char *str, char **endptr) {
  while (*str == ' ' || *str == '\t') ++str;
  double sign = 1.0;
  if (*str == '-') { sign = -1.0; ++str; }
  else if (*str == '+') ++str;
  double int_part = 0.0;
  while (*str >= '0' && *str <= '9')
    int_part = int_part * 10.0 + (double)(*str++ - '0');
  double frac = 0.0;
  if (*str == '.') {
    ++str;
    double scale = 0.1;
    while (*str >= '0' && *str <= '9') {
      frac += (double)(*str++ - '0') * scale;
      scale *= 0.1;
    }
  }
  double result = sign * (int_part + frac);
  if (*str == 'e' || *str == 'E') {
    ++str;
    int exp_sign = 1;
    if (*str == '-') { exp_sign = -1; ++str; }
    else if (*str == '+') ++str;
    int exp = 0;
    while (*str >= '0' && *str <= '9') exp = exp * 10 + (*str++ - '0');
    double factor = 1.0;
    for (int i = 0; i < exp; ++i) factor *= 10.0;
    if (exp_sign > 0) result *= factor;
    else result /= factor;
  }
  if (endptr) *endptr = (char *)str;
  return result;
}

float strtof(const char *str, char **endptr) {
  return (float)strtod(str, endptr);
}
#endif /* __FKERNEL_FREESTANDING__ */

/* getenv — no environment in freestanding context */
char *getenv(const char *name) { (void)name; return (void*)0; }
int   putenv(char *string) { (void)string; return -1; }
int   setenv(const char *name, const char *value, int overwrite) {
  (void)name; (void)value; (void)overwrite; return -1;
}
int   unsetenv(const char *name) { (void)name; return -1; }

/* rand/srand */
static unsigned int g_rand_seed = 1;
int rand(void) {
  g_rand_seed = g_rand_seed * 1103515245 + 12345;
  return (int)((g_rand_seed >> 16) & RAND_MAX);
}
void srand(unsigned int seed) { g_rand_seed = seed; }

