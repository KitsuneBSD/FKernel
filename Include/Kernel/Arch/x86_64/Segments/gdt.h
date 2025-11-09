#pragma once

#include <Kernel/Arch/x86_64/Segments/gdt_structures.h>
#include <Kernel/Arch/x86_64/Segments/tss_stacks.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

/**
 * @brief Global Descriptor Table (GDT) controller
 *
 * This singleton class manages the GDT, TSS, and associated stacks.
 * It provides initialization and allows setting the kernel stack pointer.
 */
class GDTController {
private:
  bool m_initialized = false; ///< Tracks whether GDT is initialized

  uint64_t gdt[6] = {0}; ///< Array of GDT entries
  struct TSS64 tss = {}; ///< TSS structure
  GDTR gdtr = {};        ///< GDTR structure

  void setupNull();       ///< Setup the null descriptor
  void setupKernelCode(); ///< Setup kernel code segment
  void setupKernelData(); ///< Setup kernel data segment
  void setupTSS();        ///< Setup TSS descriptor
  void setupGDT();        ///< Setup all GDT entries
  void setupGDTR();       ///< Setup GDTR structure
  void loadGDT();         ///< Load GDT using lgdt
  void loadSegments();    ///< Load segment registers
  void loadTSS();         ///< Load TSS using ltr

  GDTController() = default; ///< Private constructor for singleton

public:
  /**
   * @brief Get the singleton instance of the GDTController
   * @return Reference to GDTController
   */
  static GDTController &the() {
    static GDTController inst;
    return inst;
  }

  /**
   * @brief Initialize the GDT and TSS
   */
  void initialize();

  /**
   * @brief Set the kernel stack pointer for Ring 0
   *
   * @param stack_addr Physical address of the stack
   */
  void set_kernel_stack(uint64_t stack_addr);
};
