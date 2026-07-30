#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

inline uint64_t rdtsc_entropy() {
  uint32_t lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<uint64_t>(hi) << 32) | lo;
}

} // namespace fkernel
