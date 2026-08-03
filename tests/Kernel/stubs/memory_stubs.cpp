// Host-side stubs for the kernel memory singletons used by slab_allocator.cpp.
//
// grow_slab() calls PhysicalMemoryManager::alloc_page/alloc_contiguous and
// dereferences the returned "physical" address directly (identity-map style),
// so the stubs hand out host malloc'd blocks.  The slab's >8192-byte fallback
// calls MemoryManager::allocate/free, also backed by host malloc.
//
// The Test target compiles against the kernel's own freestanding LibC headers,
// so glibc <stdlib.h> would conflict with them; malloc/free are declared here
// instead and resolved against the host libc at link time.

#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/memory_manager.h>

extern "C" void* malloc(size_t size);
extern "C" void free(void* ptr);
extern "C" int posix_memalign(void** memptr, size_t alignment, size_t size);

uintptr_t fkernel::PhysicalMemoryManager::alloc_page(ZoneType, uint32_t) {
  void* p = nullptr;
  if (posix_memalign(&p, 4096, 4096) != 0)
    return 0;
  return reinterpret_cast<uintptr_t>(p);
}

uintptr_t fkernel::PhysicalMemoryManager::alloc_contiguous(size_t order, ZoneType,
                                                           uint32_t) {
  void* p = nullptr;
  if (posix_memalign(&p, 4096, order_to_size(order)) != 0)
    return 0;
  return reinterpret_cast<uintptr_t>(p);
}

void* fkernel::MemoryManager::allocate(size_t size) {
  return malloc(size);
}

void fkernel::MemoryManager::free(void* ptr) {
  ::free(ptr);
}
