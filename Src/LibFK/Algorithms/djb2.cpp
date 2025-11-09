#include <LibFK/Algorithms/djb2.h>

uint32_t djb2(const void *data, size_t length) {
  uint32_t hash = 5381;
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
  for (size_t i = 0; i < length; ++i) {
    hash = ((hash << 5) + hash) + bytes[i]; // hash * 33 + byte
  }
  return hash;
}
