#pragma once

#include <LibC/stdint.h>

namespace gdt {

constexpr uint8_t ACCESS_PRESENT = 0x80;
constexpr uint8_t ACCESS_PRIVL_RING0 = 0x00;
constexpr uint8_t ACCESS_PRIVL_RING3 = 0x60;
constexpr uint8_t ACCESS_SEGMENT = 0x10;
constexpr uint8_t ACCESS_EXECUTABLE = 0x08;
constexpr uint8_t ACCESS_DIRECTION =
    0x04; // Data segments: 0 = expand up, 1 = expand down
constexpr uint8_t ACCESS_READ_WRITE = 0x02;
constexpr uint8_t ACCESS_ACCESSED = 0x01;

constexpr uint8_t GRANULARITY_4K = 0x80;
constexpr uint8_t GRANULARITY_32BIT = 0x40;
constexpr uint8_t GRANULARITY_64BIT =
    0x20; // Not used in segment descriptor for 64-bit mode, but keep for
          // clarity

constexpr uint8_t SEGMENT_CODE_RING0 = ACCESS_PRESENT | ACCESS_PRIVL_RING0 |
                                       ACCESS_SEGMENT | ACCESS_EXECUTABLE |
                                       ACCESS_READ_WRITE;
constexpr uint8_t SEGMENT_DATA_RING0 =
    ACCESS_PRESENT | ACCESS_PRIVL_RING0 | ACCESS_SEGMENT | ACCESS_READ_WRITE;
constexpr uint8_t SEGMENT_CODE_RING3 = ACCESS_PRESENT | ACCESS_PRIVL_RING3 |
                                       ACCESS_SEGMENT | ACCESS_EXECUTABLE |
                                       ACCESS_READ_WRITE;
constexpr uint8_t SEGMENT_DATA_RING3 =
    ACCESS_PRESENT | ACCESS_PRIVL_RING3 | ACCESS_SEGMENT | ACCESS_READ_WRITE;

// Granularity flags combined
constexpr uint8_t GRANULARITY_FLAGS =
    GRANULARITY_4K | 0x20; // 0x20 here may be for long mode

constexpr int SELECTOR_SIZE = 8;

} // namespace gdt
