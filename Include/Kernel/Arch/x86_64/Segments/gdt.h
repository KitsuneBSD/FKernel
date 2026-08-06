#pragma once

#include <Kernel/Arch/x86_64/Segments/Gdt/gdt_structures.h>
#include <Kernel/Arch/x86_64/Segments/Tss/tss_stacks.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Types/types.h>

class GDTController {
private:
  bool m_initialized = false;

  uint64_t m_gdt_per_cpu[::MAX_CPUS][10];
  TSS64    m_tss_per_cpu[::MAX_CPUS];
  GDTR     m_gdtr_per_cpu[::MAX_CPUS];

  void setup_entries(uint64_t gdt[10]);
  void fill_tss(uint32_t cpu_index);

  GDTController() = default;

public:
  static GDTController &the() {
    static GDTController inst;
    return inst;
  }

  void initialize();
  void init_per_cpu(uint32_t cpu_index);
  void load_per_cpu(uint32_t cpu_index);
  void set_kernel_stack(uint64_t stack_addr);

  /// Unmaps the guard page below each IST stack. Must run after the VMM is up.
  void install_ist_guard_pages();
};
