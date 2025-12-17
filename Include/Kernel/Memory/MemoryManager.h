#pragma once

#ifdef __x86_64
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Memory/VirtualMemory/Pages/PageFlags.h>
#include <LibFK/Types/types.h>

/**
 * @class MemoryManager
 * @brief Manages the system's virtual memory, including page mapping and
 * virtual-to-physical address translation.
 */
class MemoryManager {
private:
  /**
   * @brief Private constructor to implement the Singleton pattern.
   */
  MemoryManager() = default;

  /**
   * @brief Deleted copy constructor to prevent copying of the Singleton
   * instance.
   */
  MemoryManager(const MemoryManager &) = delete;

  /**
   * @brief Deleted assignment operator to prevent copying of the Singleton
   * instance.
   */
  MemoryManager &operator=(const MemoryManager &) = delete;

  /**
   * @brief Indicates whether the virtual memory manager has been initialized.
   */
  bool m_is_initialized = false;
public:
  /**
   * @brief Retrieves the Singleton instance of the MemoryManager.
   * @return Reference to the Singleton instance.
   */
  static MemoryManager &the() {
    static MemoryManager instance;
    return instance;
  };

  /**
   * @brief Initializes the virtual memory manager.
   */
  void initialize(const multiboot2::TagMemoryMap *mmap);
  void map_page(uintptr_t virt, uintptr_t phys, PageFlags flags);
};
