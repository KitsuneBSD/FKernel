#include <Kernel/Boot/multiboot2.h>

#include <Kernel/Memory/PhysicalMemory/PhysicalMemoryZone.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/BuddyAllocator.h>
#include <Kernel/Memory/PhysicalMemory/Zone/ZoneAllocator.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

class PhysicalMemoryManager {
private:
  static constexpr size_t MAX_PHYSICAL_ZONES = 8;

  PhysicalZone m_zones[MAX_PHYSICAL_ZONES];
  size_t       m_zone_count{0};

  size_t m_total_memory{0};
  size_t m_free_memory{0};

  bool m_is_initialized{false};

private:
  PhysicalZone* create_zone(uintptr_t base,
                            size_t length,
                            ZoneType type,
                            uint64_t* bitmap_storage,
                            size_t bitmap_bits);

  PhysicalZone* find_zone_for_paddr(uintptr_t phys);
  PhysicalZone* select_zone(ZoneType preferred);
  void process_range(
    uintptr_t base,
    uintptr_t end,
    uint64_t*& bitmap_cursor,
    size_t bitmap_words_remaining);

public:
  PhysicalMemoryManager() = default;
  PhysicalMemoryManager(const PhysicalMemoryManager&) = delete;
  PhysicalMemoryManager& operator=(const PhysicalMemoryManager&) = delete;

  static PhysicalMemoryManager& the() {
    static PhysicalMemoryManager instance;
    return instance;
  }

  void initialize(const multiboot2::TagMemoryMap* mmap);

  /* Page-based API (bitmap-backed) */
  uintptr_t alloc_page(ZoneType preferred = ZoneType::NORMAL);
  void free_page(uintptr_t phys);

  /* Buddy-backed contiguous allocations */
  uintptr_t alloc_contiguous(size_t order,
                             ZoneType preferred = ZoneType::NORMAL);
  void free_contiguous(uintptr_t phys, size_t order);

  size_t total_memory() const { return m_total_memory; }
  size_t free_memory()  const { return m_free_memory; }
};