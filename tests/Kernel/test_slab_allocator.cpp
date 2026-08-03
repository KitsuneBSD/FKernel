#include <tests/test_framework.h>
#include <Kernel/Memory/ObjectMemory/slab_allocator.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Utilities/memory.h>

// Host-side tests for SlabAllocator::reallocate / is_slab_allocation (M4).
// PhysicalMemoryManager/MemoryManager are backed by the stubs in
// tests/Kernel/stubs/memory_stubs.cpp.

static const char* test_slab_alloc_and_free() {
  SlabAllocator& slab = SlabAllocator::the();
  slab.initialize();

  void* p = slab.allocate(16);
  TEST_ASSERT_NOT_NULL(p, "16-byte allocation succeeds");
  TEST_ASSERT(slab.is_slab_allocation(p), "slab pointer is recognized");

  TEST_ASSERT(slab.deallocate(p), "slab pointer frees through the slab");
  return nullptr;
}

static const char* test_slab_is_slab_allocation_rejects_heap_pointer() {
  SlabAllocator& slab = SlabAllocator::the();

  // A heap pointer must never be mistaken for a slab object (this is what let
  // MemoryManager::reallocate read a fake BlockHeader magic and kfatal).
  void* heap = MemoryManager::the().allocate(64);
  TEST_ASSERT_NOT_NULL(heap, "stubbed heap allocation works");
  TEST_ASSERT(!slab.is_slab_allocation(heap), "heap pointer is not a slab object");
  TEST_ASSERT(nullptr == slab.reallocate(heap, 128),
              "realloc of a non-slab pointer returns nullptr (caller keeps heap path)");
  MemoryManager::the().free(heap);
  return nullptr;
}

static const char* test_slab_realloc_grows_and_preserves_data() {
  SlabAllocator& slab = SlabAllocator::the();

  void* p = slab.allocate(16);
  TEST_ASSERT_NOT_NULL(p, "allocate 16");
  fk::memory::set(p, 0xAB, 16);

  void* grown = slab.reallocate(p, 64);
  TEST_ASSERT_NOT_NULL(grown, "realloc to 64 succeeds");
  TEST_ASSERT(grown != p, "grew into a larger cache -> new object");

  for (size_t i = 0; i < 16; i++)
    TEST_ASSERT_EQ(0xAB, static_cast<uint8_t*>(grown)[i], "old contents preserved");

  TEST_ASSERT(slab.is_slab_allocation(grown), "grown object remains slab-backed");
  TEST_ASSERT(slab.deallocate(grown), "grown object frees through the slab");

  void* reuse = slab.allocate(16);
  TEST_ASSERT_NOT_NULL(reuse, "16-byte object allocatable again");
  slab.deallocate(reuse);
  return nullptr;
}

static const char* test_slab_realloc_shrink_is_in_place() {
  SlabAllocator& slab = SlabAllocator::the();

  void* p = slab.allocate(128);
  TEST_ASSERT_NOT_NULL(p, "allocate 128");
  fk::memory::set(p, 0x11, 128);

  void* same = slab.reallocate(p, 32);
  TEST_ASSERT(same == p, "shrinking within the same cache keeps the pointer");

  TEST_ASSERT(slab.deallocate(p), "frees through the slab");
  return nullptr;
}

static const char* test_slab_realloc_crosses_cache_boundary() {
  SlabAllocator& slab = SlabAllocator::the();

  void* p = slab.allocate(16);
  TEST_ASSERT_NOT_NULL(p, "allocate 16");
  fk::memory::set(p, 0xCD, 16);

  // 9000 exceeds the 8192-byte largest cache -> falls back to the (stubbed)
  // kernel heap via MemoryManager::allocate.
  void* heap = slab.reallocate(p, 9000);
  TEST_ASSERT_NOT_NULL(heap, "realloc beyond slab ceiling falls back to heap");
  TEST_ASSERT(!slab.is_slab_allocation(heap), "fallback result is a heap pointer");
  for (size_t i = 0; i < 16; i++)
    TEST_ASSERT_EQ(0xCD, static_cast<uint8_t*>(heap)[i], "contents preserved across fallback");
  MemoryManager::the().free(heap);
  return nullptr;
}

static test_case_t s_tests[] = {
    {"slab_alloc_and_free",                    test_slab_alloc_and_free},
    {"is_slab_allocation_rejects_heap_ptr",    test_slab_is_slab_allocation_rejects_heap_pointer},
    {"realloc_grows_and_preserves_data",       test_slab_realloc_grows_and_preserves_data},
    {"realloc_shrink_is_in_place",             test_slab_realloc_shrink_is_in_place},
    {"realloc_crosses_cache_boundary",         test_slab_realloc_crosses_cache_boundary},
};

int run_kernel_slab_allocator_tests() {
  return run_tests("Kernel::SlabAllocator",
                   s_tests,
                   sizeof(s_tests) / sizeof(s_tests[0]));
}
