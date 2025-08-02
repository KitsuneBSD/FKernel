#pragma once

#include "LibC/stdint.h"

namespace FK {

typedef LibC::uint8_t byte;
typedef LibC::uint16_t word;
typedef LibC::uint32_t dword;
typedef LibC::uint64_t qword;

typedef LibC::int8_t signed_byte;
typedef LibC::int16_t signed_word;
typedef LibC::int32_t signed_dword;
typedef LibC::int64_t signed_qword;

constexpr unsigned KiB = 1024;
constexpr unsigned MiB = KiB * KiB;
constexpr unsigned GiB = MiB * KiB;
}

#define UNUSED(x) \
    do {          \
        (void)x;  \
    } while (0);
