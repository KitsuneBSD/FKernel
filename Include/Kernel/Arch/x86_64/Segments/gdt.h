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

  uint64_t gdt[8] = {0}; ///< Array of GDT entries
  struct TSS64 tss = {}; ///< TSS structure
  GDTR gdtr = {};        ///< GDTR structure
  void setupNull() {
    gdt[0] = 0;
    kdebug("GDT", "Null descriptor initialized");
  }

  void setupKernelCode() {
    gdt[1] =
        createSegment(SegmentAccess::Ring0Code,
                      SegmentFlags::LongMode | SegmentFlags::Granularity4K);
    kdebug("GDT", "Kernel code segment configured (selector=0x08)");
  }

  void setupKernelData() {
    gdt[2] =
        createSegment(SegmentAccess::Ring0Data, SegmentFlags::Granularity4K);
    kdebug("GDT", "Kernel data segment configured (selector=0x10)");
  }

  void setupUserCode() {
    gdt[3] =
        createSegment(SegmentAccess::Ring3Code,
                      SegmentFlags::LongMode | SegmentFlags::Granularity4K);
    kdebug("GDT", "User code segment configured (selector=0x18)");
  }

  void setupUserData() {
    gdt[4] =
        createSegment(SegmentAccess::Ring3Data, SegmentFlags::Granularity4K);
    kdebug("GDT", "User data segment configured (selector=0x20)");
  }
  void setupTSS();     ///< Setup TSS descriptor
  void setupGDT();     ///< Setup all GDT entries
  void setupGDTR();    ///< Setup GDTR structure
  void loadGDT();      ///< Load GDT using lgdt
  void loadSegments(); ///< Load segment registers
  void loadTSS();      ///< Load TSS using ltr

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
