#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Utilities/memory.h>

namespace fk {
namespace algorithms {

struct IoVec {
  const void* base;
  size_t len;
};

// Copies `iovcnt` scatter-gather segments into a single contiguous buffer.
// `out` must have at least `total_bytes(iov, iovcnt)` bytes of space.
static inline size_t gather_copy(const IoVec* iov, size_t iovcnt, uint8_t* out) {
  size_t offset = 0;
  for (size_t i = 0; i < iovcnt; ++i) {
    if (!iov[i].base || iov[i].len == 0) continue;
    fk::memory::copy(out + offset, iov[i].base, iov[i].len);
    offset += iov[i].len;
  }
  return offset;
}

// Returns the total byte count across all segments.
static inline size_t gather_total(const IoVec* iov, size_t iovcnt) {
  size_t total = 0;
  for (size_t i = 0; i < iovcnt; ++i)
    total += iov[i].len;
  return total;
}

} // namespace algorithms
} // namespace fk
