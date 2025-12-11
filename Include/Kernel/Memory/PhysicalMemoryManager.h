#include <Kernel/Boot/multiboot2.h>

#include <Kernel/Memory/PhysicalMemory/Buddy/BuddyAllocator.h>
#include <Kernel/Memory/PhysicalMemory/Zone/ZoneAllocator.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

class PhysicalMemoryManager {
private:
  BuddyAllocator m_buddy;
  Zone *m_zones[MAX_ZONES];

  bool m_is_initialized = false;

  uint64_t m_zone_count = 0;
  uint64_t m_total_memory = 0;
  uint64_t m_free_memory = 0;

protected:
  void register_zone(uintptr_t base_address, size_t length, ZoneType type);
  Zone *find_zone_for_addr(uintptr_t addr);
  Zone *select_zone(ZoneType preferred);

public:
  explicit PhysicalMemoryManager() = default;

  static PhysicalMemoryManager &the() {
    static PhysicalMemoryManager inst;
    return inst;
  }

  void initialize(const multiboot2::TagMemoryMap *mmap);

  uintptr_t alloc_page(ZoneType preferred = ZoneType::NORMAL);
  void free_page(uintptr_t addr);

  uintptr_t alloc_contiguous(size_t order);
  void free_contiguous(uintptr_t addr, size_t order);

  size_t total_memory() const { return m_total_memory; }
  size_t free_memory() const { return m_free_memory; }
};
