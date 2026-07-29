#pragma once

#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_table.h>

#include <LibFK/Core/error.h>
#include <LibFK/Core/result.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

class VirtualMemoryManager {
  fk::synchronization::Spinlock m_lock;
  PageTable* m_pml4 = nullptr;
  uintptr_t m_pml4_phys{0};
  uintptr_t m_kernel_pml4_phys{0};
  bool m_is_initialized{false};

  VirtualMemoryManager();
  VirtualMemoryManager(const VirtualMemoryManager&) = delete;
  VirtualMemoryManager& operator=(const VirtualMemoryManager&) = delete;
  VirtualMemoryManager(VirtualMemoryManager&&) = delete;
  VirtualMemoryManager& operator=(VirtualMemoryManager&&) = delete;

  PageTable* alloc_page_table();
  void perform_initial_identity_mapping();
  void invlpg(uintptr_t addr);
  uintptr_t get_table_virtual_address(uint16_t pml4_idx, uint16_t pdpt_idx = 0,
                                      uint16_t pd_idx = 0,
                                      uint16_t pt_idx = 0) const;

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

  /** @brief Creates a KPTI user-side shadow PML4 from a full kernel PML4.
   *  The shadow contains only user-accessible PML4 entries (User bit set) and the
   *  kernel-global identity entries needed to run the syscall trampoline in ring 0.
   *  Returns the physical address of the new PML4, or 0 on OOM.
   */
  uintptr_t create_kpti_user_pml4(uintptr_t kernel_pml4_phys);

  /** @brief Frees a KPTI user-side shadow PML4 (does NOT free the shared page tables). */
  void free_kpti_user_pml4(uintptr_t user_pml4_phys);

  void unmap_page_range(uintptr_t start, uintptr_t end);
  PageTable* ensure_table(PageTable* parent, size_t index, PageFlags flags, bool& changed);
};

} // namespace fkernel
using fkernel::VirtualMemoryManager;
