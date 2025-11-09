#pragma once

#ifdef __x86_64
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

#include <LibFK/Types/types.h>
#include <Kernel/MemoryManager/Pages/PageFlags.h>

constexpr uintptr_t align_down(uintptr_t addr, size_t size) {
  return addr & ~(size - 1);
}

constexpr uintptr_t align_up(uintptr_t addr, size_t size) {
  return (addr + size - 1) & ~(size - 1);
}

constexpr bool is_aligned(uintptr_t addr, size_t size) {
  return (addr & (size - 1)) == 0;
}

/**
 * @class VirtualMemoryManager
 * @brief Manages the system's virtual memory, including page mapping and
 * virtual-to-physical address translation.
 */
class VirtualMemoryManager {
private:
  /**
   * @brief Private constructor to implement the Singleton pattern.
   */
  VirtualMemoryManager() = default;

  /**
   * @brief Deleted copy constructor to prevent copying of the Singleton
   * instance.
   */
  VirtualMemoryManager(const VirtualMemoryManager &) = delete;

  /**
   * @brief Deleted assignment operator to prevent copying of the Singleton
   * instance.
   */
  VirtualMemoryManager &operator=(const VirtualMemoryManager &) = delete;

  /**
   * @brief Indicates whether the virtual memory manager has been initialized.
   */
  bool m_is_initialized = false;

  /**
   * @brief Pointer to the PML4 (Page Map Level 4) table used in memory paging.
   */
  uint64_t *m_pml4 = nullptr;

  /**
   * @brief Allocates a new page table.
   * @return Pointer to the allocated page table.
   */
  uint64_t *alloc_table();

public:
  /**
   * @brief Retrieves the Singleton instance of the VirtualMemoryManager.
   * @return Reference to the Singleton instance.
   */
  static VirtualMemoryManager &the() {
    static VirtualMemoryManager instance;
    return instance;
  };

  /**
   * @brief Initializes the virtual memory manager.
   */
  void initialize();

  /**
   * @brief Maps a virtual page to a physical address with the specified flags.
   * @param virt Virtual address to be mapped.
   * @param phys Corresponding physical address.
   * @param flags Configuration flags for the mapping.
   * @param page_size Size of the page to be mapped (default is 4KB).
   */
  void map_page(uintptr_t virt, uintptr_t phys, uint64_t flags,
                [[maybe_unused]] uint64_t page_size = PAGE_SIZE);

  /**
   * @brief Maps a range of virtual addresses to physical addresses.
   * @param virt_start Starting virtual address of the range.
   * @param phys_start Corresponding starting physical address.
   * @param size Size of the range to be mapped.
   * @param flags Configuration flags for the mapping.
   */
  void map_range(uintptr_t virt_start, uintptr_t phys_start, size_t size,
                 uint64_t flags);

  /**
   * @brief Converts a virtual address to its corresponding physical address.
   * @param virt Virtual address to be converted.
   * @return Corresponding physical address.
   */
  uintptr_t virt_to_phys(uintptr_t virt);
};
