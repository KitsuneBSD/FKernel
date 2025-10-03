#pragma once

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Boot/multiboot2.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Tree/rb_tree.h>
#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

extern "C" uintptr_t __kernel_start;
extern "C" uintptr_t __kernel_end;
extern "C" uintptr_t __heap_start;
extern "C" uintptr_t __heap_end;

#include <Kernel/MemoryManager/MemoryRange.h>

/**
 * @class PhysicalMemoryManager
 * @brief Manages physical memory allocation and deallocation in the system.
 *
 * This class is responsible for handling physical memory ranges and providing
 * mechanisms to allocate and free physical memory pages. It uses a red-black
 * tree to manage memory ranges efficiently.
 *
 * The class follows the singleton design pattern, ensuring that only one
 * instance exists throughout the system. It is not copyable or movable to
 * maintain the integrity of the singleton instance.
 */
class PhysicalMemoryManager {
  /**
   * @brief Grants access to VirtualMemoryManager as a friend class.
   */
  friend class VirtualMemoryManager;

private:
  /**
   * @brief A red-black tree to manage physical memory ranges.
   * 
   * The tree stores memory ranges and allows efficient allocation and
   * deallocation of memory blocks. The template parameter 65536 specifies
   * the maximum number of nodes in the tree.
   */
  rb_tree<PhysicalMemoryRange, 65536> m_memory_ranges;

  /**
   * @brief Indicates whether the memory manager has been initialized.
   */
  bool is_initialized = false;

  /**
   * @brief Private constructor to enforce the singleton pattern.
   */
  PhysicalMemoryManager() = default;

  /**
   * @brief Private destructor to prevent external destruction.
   */
  ~PhysicalMemoryManager() = default;

  /**
   * @brief Deleted copy constructor to prevent copying.
   */
  PhysicalMemoryManager(const PhysicalMemoryManager &) = delete;

  /**
   * @brief Deleted copy assignment operator to prevent copying.
   */
  PhysicalMemoryManager &operator=(const PhysicalMemoryManager &) = delete;

  /**
   * @brief Deleted move constructor to prevent moving.
   */
  PhysicalMemoryManager(PhysicalMemoryManager &&) = delete;

  /**
   * @brief Deleted move assignment operator to prevent moving.
   */
  PhysicalMemoryManager &operator=(PhysicalMemoryManager &&) = delete;

  /**
   * @brief Allocates memory from a specific node in the red-black tree.
   * 
   * @param node The node from which memory is to be allocated.
   * @param count The number of pages to allocate.
   * @param addr_hint A hint for the desired memory address.
   * @return An integer indicating the success or failure of the allocation.
   */
  int alloc_from_node(rb_node<PhysicalMemoryRange> *node, size_t count,
                      uintptr_t addr_hint);

public:
  /**
   * @brief Provides access to the singleton instance of the class.
   * 
   * @return A reference to the singleton instance of PhysicalMemoryManager.
   */
  static PhysicalMemoryManager &the() {
    static PhysicalMemoryManager instance;
    return instance;
  }

  /**
   * @brief Initializes the physical memory manager with a memory map.
   * 
   * @param mmap A pointer to the multiboot2 memory map tag.
   */
  void initialize(const multiboot2::TagMemoryMap *mmap);

  /**
   * @brief Allocates one or more physical memory pages.
   * 
   * @param count The number of pages to allocate.
   * @param addr_hint A hint for the desired memory address (default is 0).
   * @return A pointer to the allocated memory, or nullptr if allocation fails.
   */
  void *alloc_physical_page(size_t count, uintptr_t addr_hint = 0);

  /**
   * @brief Frees a previously allocated physical memory page.
   * 
   * @param page A pointer to the memory page to be freed.
   */
  void free_physical_page(void *page);
};
