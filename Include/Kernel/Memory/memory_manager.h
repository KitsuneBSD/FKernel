#pragma once

#ifdef __x86_64
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif
#include <Kernel/Memory/ObjectMemory/Zone/zone_types.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Types/types.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel { class IOMMU; }

namespace fkernel {

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
    BlockHeader *prev;
    static constexpr uint32_t MAGIC = 0xC0FFEE;
    uint32_t magic;
  };

  BlockHeader *m_heap_head = nullptr;
  bool m_heap_initialized = false;
  mutable fk::synchronization::Spinlock m_heap_lock;

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
   * @brief Translates a virtual address to its corresponding physical address.
   */
  uintptr_t translate(uintptr_t virt);

  /**
   * @brief Removes a mapping for a virtual address.
   */
  void unmap_page(uintptr_t virt);

  /**
   * @brief Allocates a physical page frame.
   */
  uintptr_t allocate_page(ZoneType preferred = ZoneType::NORMAL, uint32_t preferred_node = 0);

  /**
   * @brief Frees a physical page frame.
   */
  void free_page(uintptr_t phys);

  /**
   * @brief Allocates a contiguous range of physical memory.
   */
  uintptr_t allocate_contiguous(size_t order, ZoneType preferred = ZoneType::NORMAL, uint32_t preferred_node = 0);

  /**
   * @brief Frees a contiguous range of physical memory.
   */
  void free_contiguous(uintptr_t phys, size_t order);

  /**
   * @brief Get the active IOMMU controller if present.
   */
  fkernel::IOMMU* get_iommu() const;

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

  /**
   * @brief Returns total and free bytes in the kernel heap by walking the block list.
   */
  void heap_stats(size_t& total_out, size_t& free_out) const;
};

} // namespace fkernel
using fkernel::MemoryManager;