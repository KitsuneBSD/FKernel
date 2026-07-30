#include <Kernel/Memory/ObjectMemory/slab_allocator.h>
#include <Kernel/Memory/ObjectMemory/slab.h>
#include <Kernel/Memory/ObjectMemory/slab_cache.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Utilities/memory.h>

namespace {

static constexpr size_t SLAB_PAGE_SIZE = 4096;

static constexpr size_t CACHE_COUNT = 8;
static constexpr size_t CACHE_SIZES[CACHE_COUNT] = {
    16, 32, 64, 128, 256, 512, 1024, 2048
};

static SlabCache s_caches[CACHE_COUNT];

static size_t align_up(size_t n, size_t alignment) {
  return (n + alignment - 1) & ~(alignment - 1);
}

static size_t slab_header_size() {
  return align_up(sizeof(Slab), 16);
}

static Slab *ptr_to_slab(void *ptr) {
  uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
  return reinterpret_cast<Slab *>(addr & ~(SLAB_PAGE_SIZE - 1));
}

static void init_slab(Slab *slab, SlabCache *cache) {
  size_t header_sz = slab_header_size();
  size_t obj_sz = cache->object_size;
  size_t count = cache->objects_per_slab;

  slab->cache = cache;
  slab->next = nullptr;

  uint8_t *base = reinterpret_cast<uint8_t *>(slab) + header_sz;
  slab->free_list.initialize(base, obj_sz, count);
}

static Slab *grow_slab(SlabCache *cache) {
  uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
  if (!phys) {
    fk::algorithms::kwarn("SLAB", "grow_slab: PMM out of pages for size %zu",
                          cache->object_size);
    return nullptr;
  }

  Slab *slab = reinterpret_cast<Slab *>(phys);
  fk::memory::set(slab, 0, SLAB_PAGE_SIZE);

  init_slab(slab, cache);

  slab->next = cache->partial_slabs;
  cache->partial_slabs = slab;
  cache->total_objects += cache->objects_per_slab;
  cache->total_free += cache->objects_per_slab;

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
  if (m_is_initialized) return;

  size_t header_sz = slab_header_size();

  for (size_t i = 0; i < CACHE_COUNT; i++) {
    size_t obj_sz = CACHE_SIZES[i];
    size_t usable = SLAB_PAGE_SIZE - header_sz;
    size_t count = usable / obj_sz;

    s_caches[i].object_size = obj_sz;
    s_caches[i].objects_per_slab = count;
    s_caches[i].slab_header_offset = header_sz;
    s_caches[i].partial_slabs = nullptr;
    s_caches[i].total_objects = 0;
    s_caches[i].total_free = 0;

    fk::algorithms::klog("SLAB", "Cache %zu: objsize=%zu count=%zu waste=%zu",
                         i, obj_sz, count, usable - count * obj_sz);
  }

  m_is_initialized = true;
  fk::algorithms::klog("SLAB", "SlabAllocator initialized with %zu caches", CACHE_COUNT);
}

void *SlabAllocator::allocate(size_t size) {
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

  return obj;
}

bool SlabAllocator::deallocate(void *ptr) {
  if (!m_is_initialized || !ptr) return false;

  Slab *slab = ptr_to_slab(ptr);
  SlabCache *cache = slab->cache;

  if (!cache || cache < s_caches || cache >= s_caches + CACHE_COUNT)
    return false;

  uintptr_t first_obj = reinterpret_cast<uintptr_t>(slab) + slab_header_size();
  uintptr_t slab_end = reinterpret_cast<uintptr_t>(slab) + SLAB_PAGE_SIZE;
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

  return true;
}
