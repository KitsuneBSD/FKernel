#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

size_t utoa(uint64_t value, char *buffer, int base);

void *memcpy(void *dest, const void *src, size_t n);

void *memset(void *dest, int ch, size_t n);

int atoi(const char *str);

char *itoa(int num, char *str, int base);

size_t strlen(const char *str);

void reverse(char *str, int length);
