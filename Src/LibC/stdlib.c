#include <LibC/stdlib.h>
#include <LibC/errno.h>
#include <LibC/limits.h>
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
  const char *original_str = str;
  unsigned long result = 0;
  int sign = 1;
  while (*str == ' ' || *str == '\t') ++str;
  if (*str == '-') { sign = -1; ++str; }
  else if (*str == '+') ++str;
  if (base == 0) {
    if (*str == '0') { ++str; base = (*str == 'x' || *str == 'X') ? (++str, 16) : 8; }
    else base = 10;
  } else if (base == 16 && *str == '0' && (str[1] == 'x' || str[1] == 'X')) str += 2;
  const char *start = str;
  unsigned long cutoff = (sign == -1) ? (unsigned long)LONG_MAX + 1UL : (unsigned long)LONG_MAX;
  while (1) {
    int digit;
    if (*str >= '0' && *str <= '9') digit = *str - '0';
    else if (*str >= 'a' && *str <= 'z') digit = *str - 'a' + 10;
    else if (*str >= 'A' && *str <= 'Z') digit = *str - 'A' + 10;
    else break;
    if (digit >= base) break;
    if (result > (cutoff - (unsigned long)digit) / (unsigned long)base) {
      errno = ERANGE;
      if (endptr) *endptr = (char *)str;
      return sign == -1 ? LONG_MIN : LONG_MAX;
    }
    result = result * (unsigned long)base + (unsigned long)digit;
    ++str;
  }
  if (endptr) *endptr = (str == start) ? (char *)original_str : (char *)str;
  return sign == -1 ? -(long)result : (long)result;
}

/* strtoul */
unsigned long strtoul(const char *str, char **endptr, int base) {
  const char *original_str = str;
  unsigned long result = 0;
  while (*str == ' ' || *str == '\t') ++str;
  if (*str == '+') ++str;
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
    result = result * (unsigned long)base + (unsigned long)digit;
    ++str;
  }
  if (endptr) *endptr = (str == start) ? (char *)original_str : (char *)str;
  return result;
}

/* swap helper for qsort */
static void swap_bytes(char *a, char *b, size_t size) {
  char tmp;
  for (size_t i = 0; i < size; i++) {
    tmp = a[i]; a[i] = b[i]; b[i] = tmp;
  }
}

/* heapify subtree rooted at index i (heap of nmemb elements, base size) */
static void sift_down(char *arr, size_t i, size_t nmemb, size_t size,
                      int (*compar)(const void *, const void *)) {
  while (1) {
    size_t largest = i;
    size_t left = 2 * i + 1;
    size_t right = 2 * i + 2;
    if (left < nmemb && compar(arr + left * size, arr + largest * size) > 0)
      largest = left;
    if (right < nmemb && compar(arr + right * size, arr + largest * size) > 0)
      largest = right;
    if (largest == i) break;
    swap_bytes(arr + i * size, arr + largest * size, size);
    i = largest;
  }
}

/* qsort — heapsort: O(n log n) worst case, O(1) extra space, no stack overflow */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
  if (nmemb < 2) return;
  char *arr = (char *)base;
  for (size_t i = nmemb / 2; i-- > 0;)
    sift_down(arr, i, nmemb, size, compar);
  for (size_t end = nmemb - 1; end > 0; end--) {
    swap_bytes(arr, arr + end * size, size);
    sift_down(arr, 0, end, size, compar);
  }
}

/* bsearch */
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *)) {
  const char *lo = (const char *)base;
  const char *hi = lo + nmemb * size;
  while (lo < hi) {
    const char *mid = lo + ((hi - lo) / (2 * size)) * size;
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
  const char *original_str = str;
  unsigned long long result = 0;
  while (*str == ' ' || *str == '\t') ++str;
  if (*str == '+') ++str;
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
    result = result * (unsigned long long)base + (unsigned long long)digit;
    ++str;
  }
  if (endptr) *endptr = (str == start) ? (char *)original_str : (char *)str;
  return result;
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

/* Environment variable support */
char **environ = (char **)0;

char *getenv(const char *name) {
  if (!environ || !name) return (void *)0;
  size_t len = strlen(name);
  for (char **ep = environ; *ep; ++ep) {
    if (strncmp(*ep, name, len) == 0 && (*ep)[len] == '=')
      return (*ep) + len + 1;
  }
  return (void *)0;
}

int putenv(char *string) {
  if (!string) return -1;
  /* Find '=' to split key from value */
  size_t key_len = 0;
  while (string[key_len] && string[key_len] != '=') ++key_len;
  if (!string[key_len]) return unsetenv(string);
  /* Replace existing entry if found */
  if (environ) {
    for (char **ep = environ; *ep; ++ep) {
      if (strncmp(*ep, string, key_len) == 0 && (*ep)[key_len] == '=') {
        *ep = string;
        return 0;
      }
    }
  }
  /* Count existing entries */
  size_t count = 0;
  if (environ) while (environ[count]) ++count;
  char **new_env = (char **)malloc((count + 2) * sizeof(char *));
  if (!new_env) return -1;
  for (size_t i = 0; i < count; ++i) new_env[i] = environ[i];
  new_env[count] = string;
  new_env[count + 1] = (char *)0;
  environ = new_env;
  return 0;
}

int setenv(const char *name, const char *value, int overwrite) {
  if (!name || !value || name[0] == '\0') return -1;
  /* Check if exists */
  if (!overwrite && getenv(name)) return 0;
  size_t name_len  = strlen(name);
  size_t value_len = strlen(value);
  char *entry = (char *)malloc(name_len + value_len + 2);
  if (!entry) return -1;
  memcpy(entry, name, name_len);
  entry[name_len] = '=';
  memcpy(entry + name_len + 1, value, value_len + 1);
  return putenv(entry);
}

int unsetenv(const char *name) {
  if (!environ || !name || name[0] == '\0') return 0;
  size_t len = strlen(name);
  for (char **ep = environ; *ep; ++ep) {
    if (strncmp(*ep, name, len) == 0 && (*ep)[len] == '=') {
      /* Shift remaining entries */
      char **src = ep + 1;
      while (*src) { *ep++ = *src++; }
      *ep = (char *)0;
      return 0;
    }
  }
  return 0;
}

/* rand/srand */
static unsigned int g_rand_seed = 1;
int rand(void) {
  g_rand_seed = g_rand_seed * 1103515245 + 12345;
  return (int)((g_rand_seed >> 16) & RAND_MAX);
}
void srand(unsigned int seed) { g_rand_seed = seed; }

