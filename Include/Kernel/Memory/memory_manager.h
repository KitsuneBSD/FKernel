#pragma once

#ifdef __x86_64
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Types/types.h>
#include <LibFK/Synchronization/spinlock.h>

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

  struct alignas(16) BlockHeader {
    size_t size;
    bool is_free;
    BlockHeader *next;
    static constexpr uint32_t MAGIC = 0xC0FFEE;
    uint32_t magic;
  };

  BlockHeader *m_heap_head = nullptr;
  bool m_heap_initialized = false;
  fk::synchronization::Spinlock m_heap_lock;

public:
  /** @return The singleton instance. */
  static MemoryManager &the() {
    static MemoryManager instance;
    return instance;
  };

  /**
   * @brief Initializes the kernel heap.
   */
  void initialize_heap();

  /**
   * @brief Initializes the memory subsystem (PMM and VMM).
   */
  void initialize();

  /**
   * @brief Checks if kernel heap is initialized.
   */
  bool is_heap_initialized() const { return m_heap_initialized; }
  
  /**
   * @brief Checks if memory manager is initialized.
   */
  bool is_initialized() const { return m_is_initialized; }

  /**
   * @brief Maps a virtual page to a physical frame.
   */
  void map_page(uintptr_t virt, uintptr_t phys, PageFlags flags);

  /**
   * @brief Allocates a memory block from the kernel heap.
   */
  void *allocate(size_t size);

  /**
   * @brief Reallocates a memory block.
   */
  void *reallocate(void *ptr, size_t size);

  /**
   * @brief Frees a previously allocated memory block.
   */
  void free(void *ptr);
};