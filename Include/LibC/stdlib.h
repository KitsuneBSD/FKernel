#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory allocation */
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

/* Process control */
void abort(void) __attribute__((noreturn));
void exit(int status) __attribute__((noreturn));
int atexit(void (*func)(void));

/* Conversions */
int atoi(const char *str);
long atol(const char *str);
long strtol(const char *str, char **endptr, int base);
unsigned long strtoul(const char *str, char **endptr, int base);
long long strtoll(const char *str, char **endptr, int base);
unsigned long long strtoull(const char *str, char **endptr, int base);
#ifndef __FKERNEL_FREESTANDING__
double strtod(const char *str, char **endptr);
float  strtof(const char *str, char **endptr);
#endif

/* Math */
static inline int       abs(int x)       { return x < 0 ? -x : x; }
static inline long      labs(long x)     { return x < 0 ? -x : x; }
static inline long long llabs(long long x) { return x < 0 ? -x : x; }

/* Division with remainder */
typedef struct { int quot; int rem; }             div_t;
typedef struct { long quot; long rem; }           ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;

static inline div_t   div(int n, int d)         { return (div_t){n/d, n%d}; }
static inline ldiv_t  ldiv(long n, long d)      { return (ldiv_t){n/d, n%d}; }
static inline lldiv_t lldiv(long long n, long long d) { return (lldiv_t){n/d, n%d}; }

/* Environment variables (stubs — no environment in freestanding context) */
char *getenv(const char *name);
int   putenv(char *string);
int   setenv(const char *name, const char *value, int overwrite);
int   unsetenv(const char *name);

/* Sorting and searching */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));

/* Exit codes */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* Number limits */
#define RAND_MAX 32767

int rand(void);
void srand(unsigned int seed);

#ifdef __cplusplus
}
#endif
