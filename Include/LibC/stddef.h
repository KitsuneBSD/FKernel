#pragma once

#include <LibC/stdint.h>

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

typedef int64_t ssize_t;

#ifdef __cplusplus
#define NULL nullptr
using nullptr_t = decltype(nullptr);
#else
#define NULL ((void *)0)
#endif

#define offsetof(type, member) __builtin_offsetof(type, member)
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr) - offsetof(type, member)))
