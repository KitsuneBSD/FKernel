#pragma once

#include "../../Include/LibK/stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#define NULL nullptr
#else
#define NULL ((void *)0)
#endif

typedef unsigned long size_t;
typedef long ptrdiff_t;

#ifndef __cplusplus
typedef int wchar_t;
#endif

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#ifdef __cplusplus
}
#endif
