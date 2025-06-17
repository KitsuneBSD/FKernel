#include <MemoryManagement/bump_alloc.h>

static uint64_t bump_start = 0;
static uint64_t bump_end = 0;
static uint64_t bump_current = 0;

void *bump_alloc_aligned(size_t size, size_t alignment) {
  if (alignment == 0)
    alignment = 16;

  uint64_t aligned = (bump_current + alignment - 1) & ~(alignment - 1);
  if (aligned + size > bump_end) {
    Log(LogLevel::ERROR, "Bump allocator: out of memory");
    return NULL;
  }

  void *result = (void *)aligned;
  bump_current = aligned + size;
  return result;
}
void bump_allocator_init(uint64_t start, uint64_t size) {
  bump_start = start;
  bump_end = start + size;
  bump_current = start;

  Logf(LogLevel::INFO, "Bump Allocator initialized from 0x%lx to 0x%lx",
       bump_start, bump_end);
}

void *bump_alloc(size_t size) { return bump_alloc_aligned(size, 16); }

void *bump_allocator_current() { return (void *)bump_current; }

void bump_allocator_reset() {
  bump_current = bump_start;
  Log(LogLevel::WARN, "Bump allocator reset requested");
}
