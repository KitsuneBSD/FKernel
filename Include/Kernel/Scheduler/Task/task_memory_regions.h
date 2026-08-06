#pragma once

#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Types/types.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>

struct TaskMemoryRegions {
  uintptr_t heap_start{0};
  uintptr_t heap_break{0};
  uintptr_t mmap_start{0};
  uintptr_t mmap_end{0};
  uint64_t  pf_count{0};
  uint64_t  pf_window_ticks{0};
  fk::containers::Vector<::fkernel::MemoryRegion> list;
};
