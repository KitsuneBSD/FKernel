#pragma once

#include <LibFK/Types/types.h>

enum SegmentAccess : uint8_t {
  Ring0Code = 0x9A,
  Ring0Data = 0x92,
  Ring3Code = 0xFA,
  Ring3Data = 0xF2,
  TSS64Type = 0x89
};

constexpr SegmentAccess operator|(SegmentAccess a, SegmentAccess b) {
  return static_cast<SegmentAccess>(static_cast<uint8_t>(a) |
                                    static_cast<uint8_t>(b));
}
