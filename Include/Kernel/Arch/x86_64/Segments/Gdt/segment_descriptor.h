#pragma once

#include <Kernel/Arch/x86_64/Segments/Gdt/segment_access.h>
#include <Kernel/Arch/x86_64/Segments/Gdt/segment_flags.h>

constexpr uint64_t createSegment(SegmentAccess access, SegmentFlags flags) {
  uint64_t entry = 0;
  entry |= 0xFFFFULL;
  entry |= (uint64_t)access << 40;
  entry |= (uint64_t)(flags & 0xF0) << 48;
  entry |= (uint64_t)0xF << 48;
  return entry;
}
