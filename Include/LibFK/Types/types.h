#pragma once

#include <LibC/stdint.h>
#include <LibC/stddef.h>
#include <LibC/stdbool.h>
#include <LibC/limits.h>

constexpr uint64_t KiB = 1024;
constexpr uint64_t MiB = 1024 * KiB;
constexpr uint64_t GiB = 1024 * MiB;
constexpr uint64_t TiB = 1024 * GiB;
constexpr uint64_t PiB = 1024 * TiB;
constexpr uint64_t EiB = 1024 * PiB;

constexpr uint64_t KB = 1000;
constexpr uint64_t MB = 1000 * KB;
constexpr uint64_t GB = 1000 * MB;
constexpr uint64_t TB = 1000 * GB;
constexpr uint64_t PB = 1000 * TB;
constexpr uint64_t EB = 1000 * PB;

namespace fk {
template <typename T>
constexpr T &&move(T &arg) {
  return static_cast<T &&>(arg);
}
} // namespace fk
