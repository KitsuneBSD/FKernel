#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace utilities {

inline uint8_t bcd_to_bin(uint8_t bcd) {
  return (bcd & 0x0F) + ((bcd / 16) * 10);
}

} // namespace utilities
} // namespace fk