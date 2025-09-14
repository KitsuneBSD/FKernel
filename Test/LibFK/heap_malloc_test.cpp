#include "Tests/LibFK/heap_malloc_test.h"
#include <LibC/stdint.h>
#include <LibFK/heap_malloc.h>
#include <Tests/test_runner.h>

static uint8_t test_heap[32 * 1024 * 1024]; // 32 MiB
static Allocator t_allocator;

extern "C" {
uint8_t *__heap_start = test_heap;
uint8_t *__heap_end = test_heap + sizeof(test_heap);
}

static void init_test_heap() {
  t_allocator.initialize(test_heap, test_heap + sizeof(test_heap));
}

static uint8_t *t_allocate(size_t size) {
  if (!size)
    return nullptr;

  if (size <= 8)
    return t_allocator.alloc8.allocate();
  if (size <= 16)
    return t_allocator.alloc16.allocate();
  if (size <= 4096)
    return t_allocator.alloc4096.allocate();
  if (size <= 16384)
    return t_allocator.alloc16384.allocate();
  return nullptr;
}

static uint8_t *t_allocate_zeroed(size_t size) {
  uint8_t *p = t_allocate(size);
  if (!p)
    return nullptr;
  memset(p, 0, size);
  return p;
}

static void t_free(uint8_t *ptr) {
  if (!ptr)
    return;
  if (t_allocator.alloc8.isInAllocator(ptr)) {
    t_allocator.alloc8.free(ptr);
    return;
  }
  if (t_allocator.alloc16.isInAllocator(ptr)) {
    t_allocator.alloc16.free(ptr);
    return;
  }
  if (t_allocator.alloc4096.isInAllocator(ptr)) {
    t_allocator.alloc4096.free(ptr);
    return;
  }
  if (t_allocator.alloc16384.isInAllocator(ptr)) {
    t_allocator.alloc16384.free(ptr);
    return;
  }
}

static uint8_t *t_reallocate(uint8_t *ptr, size_t size) {
  if (!ptr)
    return t_allocate(size);
  if (size == 0) {
    t_free(ptr);
    return nullptr;
  }

  auto try_move = [&](auto &pool) -> uint8_t * {
    if (pool.isInAllocator(ptr)) {
      size_t old = pool.chunk_size();
      if (size <= old)
        return ptr;
      uint8_t *newptr = t_allocate(size);
      if (!newptr)
        return nullptr;
      memcpy(newptr, ptr, old);
      pool.free(ptr);
      return newptr;
    }
    return nullptr;
  };

  if (auto *p = try_move(t_allocator.alloc8))
    return p;
  if (auto *p = try_move(t_allocator.alloc16))
    return p;
  if (auto *p = try_move(t_allocator.alloc4096))
    return p;
  if (auto *p = try_move(t_allocator.alloc16384))
    return p;

  return nullptr;
}

// Testes
static void test_kmalloc_basic() {
  uint8_t *p8 = t_allocate(8);
  check_bool(p8 != nullptr, true, "kmalloc_8_nonnull");

  uint8_t *p16 = t_allocate(16);
  check_bool(p16 != nullptr, true, "kmalloc_16_nonnull");

  uint8_t *p4096 = t_allocate(4096);
  check_bool(p4096 != nullptr, true, "kmalloc_4096_nonnull");

  uint8_t *p16384 = t_allocate(16384);
  check_bool(p16384 != nullptr, true, "kmalloc_16384_nonnull");

  t_free(p8);
  t_free(p16);
  t_free(p4096);
  t_free(p16384);
}

static void test_kcalloc_basic() {
  uint8_t *p = t_allocate_zeroed(16); // 16 bytes
  check_bool(p != nullptr, true, "kcalloc_nonnull");

  bool zeroed = true;
  for (size_t i = 0; i < 16; ++i) {
    if (p[i] != 0) {
      zeroed = false;
      break;
    }
  }
  check_bool(zeroed, true, "kcalloc_zeroed");
  t_free(p);
}

static void test_krealloc_basic() {
  uint8_t *p = t_allocate(8);
  check_bool(p != nullptr, true, "krealloc_initial_alloc");

  for (size_t i = 0; i < 8; ++i)
    p[i] = static_cast<uint8_t>(i);

  uint8_t *p2 = t_reallocate(p, 16);
  check_bool(p2 != nullptr, true, "krealloc_expand");

  bool preserved = true;
  for (size_t i = 0; i < 8; ++i) {
    if (p2[i] != i) {
      preserved = false;
      break;
    }
  }
  check_bool(preserved, true, "krealloc_preserve_data");

  t_free(p2);
}

extern "C" void test_heap_malloc() {
  init_test_heap();
  test_kmalloc_basic();
  test_kcalloc_basic();
  test_krealloc_basic();
  printf("All heap_malloc unit tests finished.\n");
}
