#pragma once

#ifdef __x86_64
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Types/types.h>

/**
 * @class MemoryManager
 * @brief Singleton class that coordinates virtual and physical memory managers.
 */
class MemoryManager {
private:
  /** @brief Private constructor for Singleton pattern. */
  MemoryManager() = default;

  MemoryManager(const MemoryManager &) = delete;
  MemoryManager &operator=(const MemoryManager &) = delete;

  bool m_is_initialized = false;

public:
  /** @return The singleton instance. */
  static MemoryManager &the() {
    static MemoryManager instance;
    return instance;
  };

  /**
   * @brief Initializes the memory subsystem (PMM and VMM).
   */
  void initialize();

  /**
   * @brief Maps a virtual page to a physical frame.
   */
  void map_page(uintptr_t virt, uintptr_t phys, PageFlags flags);
};
