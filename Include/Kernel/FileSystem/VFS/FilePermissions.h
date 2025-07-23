#pragma once

#include <LibC/stdint.h>

namespace FileSystem {

constexpr LibC::uint32_t O_WRONLY = 1 << 0;
constexpr LibC::uint32_t O_RDWR = 1 << 1;
constexpr LibC::uint32_t O_TRUNC = 1 << 2;

}
