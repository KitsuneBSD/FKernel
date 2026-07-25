#pragma once

#include <LibC/stdint.h>

#define INT8_MIN -128
#define INT8_MAX 127
#define UINT8_MAX 0xFF

#define INT16_MIN -32768
#define INT16_MAX 32767
#define UINT16_MAX 0xFFFF

#define INT32_MIN -2147483648
#define INT32_MAX 2147483647
#define UINT32_MAX 0xFFFFFFFFu

#define INT64_MIN (-9223372036854775807LL - 1)
#define INT64_MAX 9223372036854775807LL
#define UINT64_MAX 0xFFFFFFFFFFFFFFFFull

#define INT_MIN INT32_MIN
#define INT_MAX INT32_MAX
#define UINT_MAX UINT32_MAX

#define UINTPTR_MAX UINT64_MAX
#define INTPTR_MIN INT64_MIN
#define INTPTR_MAX INT64_MAX

#define SIZE_MAX UINT64_MAX

#define CHAR_BIT  8
#define CHAR_MIN  (-128)
#define CHAR_MAX  127
#define UCHAR_MAX 255

#define SHRT_MIN  INT16_MIN
#define SHRT_MAX  INT16_MAX
#define USHRT_MAX UINT16_MAX

#define LONG_MIN  (-9223372036854775807L - 1)
#define LONG_MAX  9223372036854775807L
#define ULONG_MAX 0xFFFFFFFFFFFFFFFFul

#define LLONG_MIN INT64_MIN
#define LLONG_MAX INT64_MAX
#define ULLONG_MAX UINT64_MAX

#define SSIZE_MAX INT64_MAX
#define NAME_MAX  255
#define PATH_MAX  4096
#define PIPE_BUF  4096

// Access mode flags for access()
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1