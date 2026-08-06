#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <Kernel/Scheduler/Task/task_memory_regions.h>

struct TaskMemory {
  uintptr_t cr3{0};
  uintptr_t prev_cr3{0};
  uintptr_t shadow_cr3{0};
  TaskMemoryRegions regions{};
};
