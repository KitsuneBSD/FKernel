#include "Log.h"
#include <Kernel/MemoryManagement/Memory_management.h>
#include <Kernel/MemoryManagement/bump_alloc.h>

static AllocatorType current_allocator = AllocatorType::Bump;

void init_memory_management(uint64_t heap_start, uint64_t heap_size) {
  bump_allocator_init(heap_start, heap_size);
  current_allocator = AllocatorType::Bump;
}

void switch_allocator(AllocatorType type) {
  if (current_allocator == type) {
    Logf(LogLevel::WARN, "You can't switch to the same allocator type.");
    return;
  }

  switch (type) {
  case AllocatorType::Bump:
    Log(LogLevel::INFO, "Switching to bump allocator");
    current_allocator = AllocatorType::Bump;
    break;

  case AllocatorType::TLSF:
    Log(LogLevel::INFO, "Switching to TLSF allocator");
    current_allocator = AllocatorType::TLSF;
    break;

  default:
    Logf(LogLevel::ERROR, "Unknown allocator type: %d", static_cast<int>(type));
    break;
  }
}

void *kmalloc(size_t size) {
  switch (current_allocator) {
  case AllocatorType::Bump:
    Logf(LogLevel::INFO, "Allocating %lu using bump allocator", size);
    return bump_alloc(size);
  case AllocatorType::TLSF:
    Logf(LogLevel::INFO, "Allocating %lu using tlsf allocator", size);
    return nullptr;
  default:
    Logf(LogLevel::WARN, "We can't allocate without unknown allocator");
    return nullptr;
  }
}

void kfree(void *ptr) {
  if (!ptr) {
    Logf(LogLevel::WARN, "We can't free a empty pointer");
    return;
  }
  if (current_allocator == AllocatorType::Bump) {
    Logf(LogLevel::WARN, "Bump Allocator wasn't a free method");
    return;
  }

  // tlsf_free(ptr);
}
