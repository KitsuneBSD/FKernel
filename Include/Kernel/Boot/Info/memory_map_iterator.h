#pragma once

#include <Kernel/Boot/Info/memory_map_entry.h>

namespace boot {

class MemoryMapIterator {
public:
  virtual ~MemoryMapIterator() = default;
  virtual bool has_next() const = 0;
  virtual MemoryMapEntry next() = 0;
  virtual void reset() = 0;
};

} // namespace boot
