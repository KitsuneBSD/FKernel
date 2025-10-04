#pragma once

#include <LibC/stdint.h>

constexpr int8_t INT8_MIN = -128;
constexpr int8_t INT8_MAX = 127;
constexpr uint8_t UINT8_MAX = 0xFF;

constexpr int16_t INT16_MIN = -32768;
constexpr int16_t INT16_MAX = 32767;
constexpr uint16_t UINT16_MAX = 0xFFFF;

constexpr int32_t INT32_MIN = -2147483648;
constexpr int32_t INT32_MAX = 2147483647;
constexpr uint32_t UINT32_MAX = 0xFFFFFFFFu;

constexpr int64_t INT64_MIN = (-9223372036854775807LL - 1);
constexpr int64_t INT64_MAX = 9223372036854775807LL;
constexpr uint64_t UINT64_MAX = 0xFFFFFFFFFFFFFFFFull;

constexpr int INT_MIN = INT32_MIN;
constexpr int INT_MAX = INT32_MAX;
constexpr unsigned UINT_MAX = UINT32_MAX;

constexpr uintptr_t UINTPTR_MAX = UINT64_MAX;
constexpr intptr_t INTPTR_MIN = INT64_MIN;
constexpr intptr_t INTPTR_MAX = INT64_MAX;
