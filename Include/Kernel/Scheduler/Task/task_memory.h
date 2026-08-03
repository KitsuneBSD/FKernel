#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Container/Sequence/vector.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>

struct TaskMemory {
  uintptr_t cr3{0};
  uintptr_t prev_cr3{0};
  struct {
    uintptr_t heap_start{0};
    uintptr_t heap_break{0};
    uintptr_t mmap_start{0};
    uintptr_t mmap_end{0};
    fk::containers::Vector<::fkernel::MemoryRegion> list;
  } regions{};
};
