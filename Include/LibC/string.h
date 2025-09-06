#pragma once

#include <LibC/stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static const char digits[] = "0123456789abcdef";

void *memmove(void *dest, const void *src, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
char *strchr(const char *s, int c);
char *strcat(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

int itoa(int val, char *buf, int base);
int atoi(const char *str);
long stol(const char *str);

#ifdef __cplusplus
}
#endif
