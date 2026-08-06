#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Utilities/aligner.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Memory/ObjectMemory/slab_allocator.h>
#include <Kernel/Memory/ObjectMemory/slab.h>
#include <Kernel/Memory/ObjectMemory/slab_cache.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/memory_manager.h>

namespace {

static constexpr size_t SLAB_PAGE_SIZE = 4096;

static constexpr size_t CACHE_COUNT = 10;
static constexpr size_t CACHE_SIZES[CACHE_COUNT] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192
};

static SlabCache s_caches[CACHE_COUNT];

static size_t slab_header_size() {
  return fk::utilities::align_up(sizeof(Slab), 16);
}

// Object pointer is always in the first page of the slab (header < 4KB,
// first object starts at header_offset), so rounding to the first 4KB boundary
// always yields the slab base — even for multi-page slabs.
static Slab *ptr_to_slab(void *ptr) {
  uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
  return reinterpret_cast<Slab *>(addr & ~(SLAB_PAGE_SIZE - 1));
}

static void init_slab(Slab *slab, SlabCache *cache) {
  size_t slab_size = SLAB_PAGE_SIZE << cache->pages_order;
  size_t header_sz = slab_header_size();
  size_t obj_sz = cache->object_size;
  size_t count = (slab_size - header_sz) / obj_sz;

  slab->cache = cache;
  slab->next = nullptr;

  uint8_t *base = reinterpret_cast<uint8_t *>(slab) + header_sz;
  slab->free_list.initialize(base, obj_sz, count);
}

static Slab *grow_slab(SlabCache *cache) {
  uintptr_t phys;
  if (cache->pages_order == 0) {
    phys = PhysicalMemoryManager::the().alloc_page();
  } else {
    size_t slab_size = SLAB_PAGE_SIZE << cache->pages_order;
    phys = PhysicalMemoryManager::the().alloc_contiguous(size_to_order(slab_size));
  }
  if (!phys) {
    fk::algorithms::kwarn("SLAB", "grow_slab: out of memory for size %zu", cache->object_size);
    return nullptr;
  }

  size_t slab_size = SLAB_PAGE_SIZE << cache->pages_order;
  Slab *slab = reinterpret_cast<Slab *>(phys);
  fk::memory::set(slab, 0, slab_size);

  init_slab(slab, cache);

  slab->next = cache->partial_slabs;
  cache->partial_slabs = slab;
  cache->total_objects += cache->objects_per_slab;
  cache->total_free += cache->objects_per_slab;

  fk::algorithms::ktrace("SLAB", "grow_slab: objsize=%zu new slab @%p count=%zu", cache->object_size, slab, cache->objects_per_slab);
  return slab;
}

static SlabCache *find_cache_for_size(size_t size) {
  for (size_t i = 0; i < CACHE_COUNT; i++) {
    if (size <= s_caches[i].object_size)
      return &s_caches[i];
  }
  return nullptr;
}

} // anonymous namespace

void SlabAllocator::initialize() {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  if (m_is_initialized) return;

  size_t header_sz = slab_header_size();

  for (size_t i = 0; i < CACHE_COUNT; i++) {
    size_t obj_sz = CACHE_SIZES[i];

    // Compute minimum page order such that (pages * 4096 - header) >= obj_sz
    uint8_t order = 0;
    while (((SLAB_PAGE_SIZE << order) - header_sz) < obj_sz)
      ++order;

    size_t slab_sz = SLAB_PAGE_SIZE << order;
    size_t usable = slab_sz - header_sz;
    size_t count = usable / obj_sz;

    s_caches[i].object_size = obj_sz;
    s_caches[i].objects_per_slab = count;
    s_caches[i].pages_order = order;
    s_caches[i].slab_header_offset = header_sz;
    s_caches[i].partial_slabs = nullptr;
    s_caches[i].total_objects = 0;
    s_caches[i].total_free = 0;

    fk::algorithms::klog("SLAB", "Cache %zu: objsize=%zu order=%u count=%zu waste=%zu",
                         i, obj_sz, (unsigned)order, count, usable - count * obj_sz);
  }

  m_is_initialized = true;
  fk::algorithms::klog("SLAB", "SlabAllocator initialized (%zu caches, max %zu B)",
                       CACHE_COUNT, CACHE_SIZES[CACHE_COUNT - 1]);
}

void *SlabAllocator::allocate(size_t size) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  return allocate_locked(size);
}

void *SlabAllocator::allocate_locked(size_t size) {
  if (!m_is_initialized || size == 0) return nullptr;

  SlabCache *cache = find_cache_for_size(size);
  if (!cache) {
    return MemoryManager::the().allocate(size);
  }

  if (!cache->partial_slabs) {
    if (!grow_slab(cache)) {
      fk::algorithms::kerror("SLAB", "allocate: out of memory for size %zu", size);
      return nullptr;
    }
  }

  Slab *slab = cache->partial_slabs;
  void *obj = slab->free_list.pop();
  cache->total_free--;

  if (slab->free_list.is_empty()) {
    cache->partial_slabs = slab->next;
    slab->next = nullptr;
  }

  fk::algorithms::ktrace("SLAB", "allocate(%zu) -> %p (slab=%p free=%zu)", size, obj, slab, cache->total_free);
  return obj;
}

bool SlabAllocator::is_slab_allocation(void *ptr) const {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  return is_slab_allocation_locked(ptr);
}

bool SlabAllocator::is_slab_allocation_locked(void *ptr) const {
  if (!m_is_initialized || !ptr) return false;

  Slab *slab = ptr_to_slab(ptr);
  SlabCache *cache = slab->cache;
  return cache >= s_caches && cache < s_caches + CACHE_COUNT;
}

void *SlabAllocator::reallocate(void *ptr, size_t size) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  return reallocate_locked(ptr, size);
}

void *SlabAllocator::reallocate_locked(void *ptr, size_t size) {
  if (!m_is_initialized || !ptr) return nullptr;

  Slab *slab = ptr_to_slab(ptr);
  SlabCache *cache = slab->cache;
  if (cache < s_caches || cache >= s_caches + CACHE_COUNT)
    return nullptr;

  size_t old_size = cache->object_size;
  if (size <= old_size)
    return ptr;

  void *new_obj = allocate_locked(size);
  if (!new_obj)
    return nullptr;

  fk::memory::copy(new_obj, ptr, old_size);
  deallocate_locked(ptr);
  return new_obj;
}

bool SlabAllocator::deallocate(void *ptr) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  return deallocate_locked(ptr);
}

bool SlabAllocator::deallocate_locked(void *ptr) {
  if (!m_is_initialized || !ptr) return false;
  if (!is_slab_allocation_locked(ptr)) return false;

  Slab *slab = ptr_to_slab(ptr);
  SlabCache *cache = slab->cache;

  uintptr_t first_obj = reinterpret_cast<uintptr_t>(slab) + slab_header_size();
  uintptr_t slab_end = reinterpret_cast<uintptr_t>(slab) + (SLAB_PAGE_SIZE << cache->pages_order);
  if (reinterpret_cast<uintptr_t>(ptr) < first_obj ||
      reinterpret_cast<uintptr_t>(ptr) >= slab_end)
    return false;

  bool was_full = slab->free_list.is_empty();

  slab->free_list.push(ptr);
  cache->total_free++;

  if (was_full) {
    slab->next = cache->partial_slabs;
    cache->partial_slabs = slab;
  }

  fk::algorithms::ktrace("SLAB", "deallocate(%p): objsize=%zu free=%zu", ptr, cache->object_size, cache->total_free);
  return true;
}
