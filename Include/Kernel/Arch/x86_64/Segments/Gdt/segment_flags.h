#pragma once

#include <LibFK/Types/types.h>

enum SegmentFlags : uint8_t {
  Granularity4K = 1 << 7,
  DefaultSize32 = 1 << 6,
  LongMode = 1 << 5,
  DefaultSize16 = 0
};

constexpr SegmentFlags operator|(SegmentFlags a, SegmentFlags b) {
  return static_cast<SegmentFlags>(static_cast<uint8_t>(a) |
                                    static_cast<uint8_t>(b));
}
