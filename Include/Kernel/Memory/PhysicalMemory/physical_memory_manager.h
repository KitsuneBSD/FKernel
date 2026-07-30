#include <Kernel/Boot/Multiboot/multiboot2.h>

#include <Kernel/Memory/ObjectMemory/Zone/zone_allocator.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_allocator.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_zone.h>

#include <LibFK/Synchronization/spinlock.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

namespace fkernel {

/**
 * @class PhysicalMemoryManager
 * @brief Singleton class that manages all physical memory zones in the system.
 */
class PhysicalMemoryManager {
private:
  fk::synchronization::Spinlock m_lock;
  PhysicalZone m_zones[MAX_PHYSICAL_ZONES]; ///< Array of available memory zones.
  size_t m_zone_count{0};

  size_t m_total_memory{0}; ///< Total usable RAM detected.
  size_t m_free_memory{0};  ///< Currently available RAM.

  bool m_is_initialized{false};

private:
  /** @brief Creates and initializes a new memory zone. */
  PhysicalZone *create_zone(uintptr_t base, size_t length, ZoneType type,
                            uint64_t *bitmap_storage, size_t bitmap_bits);

  /** @brief Finds the zone containing a specific physical address. */
  PhysicalZone *find_zone_for_paddr(uintptr_t phys);

  /** @brief Selects a zone based on type preference and availability. */
  PhysicalZone *select_zone(ZoneType preferred, uint32_t preferred_node = 0);

  /** @brief Processes a raw memory range into one or more zones. */
  void process_range(uintptr_t base, uintptr_t end, uint64_t *&bitmap_cursor,
                     size_t bitmap_words_remaining);

  /** @brief Internal single-page alloc bypassing init guard (used during boot). */
  uintptr_t alloc_page_internal(ZoneType preferred, uint32_t preferred_node);

  /** @brief Internal contiguous alloc bypassing init guard (used during boot). */
  uintptr_t alloc_contiguous_internal(size_t order, ZoneType preferred,
                                      uint32_t preferred_node);

  PhysicalMemoryManager() = default;
  PhysicalMemoryManager(const PhysicalMemoryManager &) = delete;
  PhysicalMemoryManager &operator=(const PhysicalMemoryManager &) = delete;

public:

  /** @return The singleton instance. */
  static PhysicalMemoryManager &the() {
    static PhysicalMemoryManager instance;
    return instance;
  }

  bool is_initialized() const { return m_is_initialized; }

  /**
   * @brief Scans the system memory map and initializes the manager.
   */
  void initialize();

  /**
   * @brief Reconciles buddy allocators with bitmap after direct map is available.
   */
  void reconcile_buddies();

  /**
   * @brief Mark a physical memory range as reserved (not for general allocation)
   * @param base Physical base address
   * @param length Length of range in bytes
   */
  void reserve_range(uintptr_t base, size_t length);

  /**
   * @brief Allocates a single 4KB page.
   * @param preferred The zone type preferred for this allocation.
   * @param preferred_node The preferred NUMA node ID (proximity domain).
   * @return Physical address of the allocated page.
   */
  uintptr_t alloc_page(ZoneType preferred = ZoneType::NORMAL, uint32_t preferred_node = 0);

  /** @brief Frees a previously allocated page (refcount-aware). */
  void free_page(uintptr_t phys);

  /**
   * @brief Increments the CoW reference count for a physical frame.
   * Called when a frame is shared between parent and child during fork.
   */
  void increment_refcount(uintptr_t phys);

  /**
   * @brief Decrements the CoW reference count for a physical frame.
   * @return The new refcount after decrement. Caller should free if 0.
   */
  uint16_t decrement_refcount(uintptr_t phys);

  /** @brief Returns the current CoW refcount for a physical frame. */
  uint16_t get_refcount(uintptr_t phys) const;

  /**
   * @brief Allocates a contiguous range of blocks using the buddy system.
   * @param order Power-of-two order.
   * @param preferred The zone type preferred.
   * @param preferred_node The preferred NUMA node ID.
   * @return Physical address of the block start.
   */
  uintptr_t alloc_contiguous(size_t order,
                             ZoneType preferred = ZoneType::NORMAL,
                             uint32_t preferred_node = 0);

  /** @brief Frees a contiguous block. */
  void free_contiguous(uintptr_t phys, size_t order);

  /** @return Total system RAM in bytes. */
  size_t total_memory() const { return m_total_memory; }

  /** @return Total free RAM in bytes. */
  size_t free_memory() const { return m_free_memory; }
};

} // namespace fkernel
using fkernel::PhysicalMemoryManager;
