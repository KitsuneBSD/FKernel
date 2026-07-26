#pragma once

#include <LibFK/Types/types.h>

static inline size_t read_from_buf(const char* buf, size_t len, uint64_t offset, size_t size, uint8_t* buffer) {
  if (offset >= len) return 0;
  size_t available = len - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = static_cast<uint8_t>(buf[(size_t)offset + i]);
  return to_copy;
}
