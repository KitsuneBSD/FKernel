#pragma once

#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_table.h>

#include <LibFK/Core/error.h>
#include <LibFK/Core/result.h>
#include <LibFK/Synchronization/spinlock.h>

extern "C" void write_on_cr3(void *pml4_virt_addr);
extern "C" uintptr_t read_on_cr3();
extern "C" int invalid_tlb(uintptr_t addr);

/**
 * @class VirtualMemoryManager
 * @brief Manages virtual address spaces and page table mappings for x86_64.
 */
class VirtualMemoryManager {
private:
  fk::synchronization::Spinlock m_lock;
  PageTable *m_pml4 = nullptr; ///< Pointer to the active PML4 table.
  uintptr_t m_pml4_phys = 0;   ///< Physical address of the PML4.
  uintptr_t m_kernel_pml4_phys = 0; ///< Physical address of the kernel's PML4 (never freed).

protected:
  /** @brief Allocates and zeroes a new page table. */
  PageTable *alloc_page_table();

  /** @brief Performs identity mapping for the lower kernel regions. */
  void perform_initial_identity_mapping();

  /** @brief Invalidates a single TLB entry. */
  void invlpg(uintptr_t addr);

  /** @brief (Internal) Calculates the virtual address of a page table. */
  uintptr_t get_table_virtual_address(uint16_t pml4_idx, uint16_t pdpt_idx = 0,
                                      uint16_t pd_idx = 0,
                                      uint16_t pt_idx = 0) const;

  bool m_is_initialized{false};

  VirtualMemoryManager();
  VirtualMemoryManager(const VirtualMemoryManager &) = delete;
  VirtualMemoryManager &operator=(const VirtualMemoryManager &) = delete;

public:
  /** @return The singleton instance. */
  static VirtualMemoryManager &the();

  bool is_initialized() const { return m_is_initialized; }

  /**
   * @brief Initializes the virtual memory manager, sets up PML4 and initial
   * mappings.
   */
  void initialize();

  /**
   * @brief Maps a single virtual page to a physical frame.
   */
  void map_page(uintptr_t virt, uintptr_t phys, PageFlags flags);

  /** @brief Maps a range of pages. */
  void map_range(uintptr_t start, uintptr_t size, PageFlags flags);

  /** @brief Removes a mapping for a virtual address. */
  void unmap_page(uintptr_t virt);

  /** @brief Changes the protection flags of a mapped page (RELRO, mprotect). */
  void protect_page(uintptr_t virt, PageFlags flags);

  /** @brief Translates a virtual address to its corresponding physical address.
   */
  uintptr_t translate(uintptr_t virt);

  /** @brief Gets the flags for a mapped virtual page. */
  fk::core::Result<PageFlags, fk::core::Error> get_page_flags(uintptr_t virt);

  /** @brief Creates a new address space (useful for execve). */
  uintptr_t create_address_space();

  /** @brief Clones an address space for process forking. */
  uintptr_t clone_address_space(uintptr_t source_cr3);

  /** @brief Switches the current CPU address space by updating CR3. */
  void switch_address_space(uintptr_t cr3);

  /** @brief Frees all user-space physical pages and page tables for a given CR3. */
  void free_address_space(uintptr_t cr3);

  /** @brief Unmaps a region of virtual memory. */
  fk::core::Result<int, fk::core::Error> munmap(uintptr_t addr, size_t length);

  /** @brief Extends the higher-half direct map to cover all physical memory using 2MB pages. */
  void extend_direct_map();

  /** @brief Returns the physical address of the kernel PML4. */
  uintptr_t get_kernel_cr3() const { return m_kernel_pml4_phys; }

  /** @brief Gets the PTE for a virtual address. */
  uint64_t* get_pte(uintptr_t virt, bool create = false);

  /** @brief Flushes the entire TLB by reloading CR3. Public so fork can flush after CoW marking. */
  void flush_tlb();

private:
  void unmap_page_range(uintptr_t start, uintptr_t end);

  /** @brief Ensures a page table level exists, creating it if necessary. */
  PageTable* ensure_table(PageTable* parent, size_t index, PageFlags flags, bool& changed);
};
