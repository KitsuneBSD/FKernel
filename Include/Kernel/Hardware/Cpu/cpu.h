#pragma once

#include <LibFK/Algorithms/log.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Hardware/Cpu/cpu_context.h>
#include <Kernel/Hardware/Cpu/cpu_register.h>

/**
 * @brief CPU feature detection and MSR access
 *
 * Provides a singleton interface for querying CPU features (like APIC and
 * hypervisor support) and reading/writing Model-Specific Registers (MSRs).
 */
class CPU {
private:
  fk::text::String m_vendor;
  fk::text::String m_brand;
  bool m_has_apic = false;
  bool m_has_x2apic = false;
  bool m_has_hpet = false;
  bool m_has_nx = false;
  bool m_has_smep = false;
  bool m_has_smap = false;
  bool m_has_xsave = false;
  bool m_has_avx = false;
  bool m_has_avx2 = false;
  bool m_has_avx512 = false;
  bool m_has_1gb_pages = false;
  bool m_has_invpcid = false;
  bool m_has_fsgsbase = false;
  bool m_has_umip = false;
  bool m_has_la57 = false;
  bool m_has_cet = false;
  uint8_t m_phys_addr_width = 36;
  uint8_t m_virt_addr_width = 48;
  uint8_t m_lapic_id = 0;

  void cpuid(uint32_t eax, uint32_t ecx, uint32_t *a, uint32_t *b, uint32_t *c,
             uint32_t *d);
  void detect_cpu_features();

public:
  static CPU &the() {
    static CPU inst;
    return inst;
  }

  CPU();

  fk::text::String get_vendor() const { return m_vendor; }
  fk::text::String get_brand() const { return m_brand; }
  bool has_apic() const { return m_has_apic; }
  bool has_x2apic() const { return m_has_x2apic; }
  bool has_hpet() const { return m_has_hpet; }
  bool has_nx() const { return m_has_nx; }
  bool has_smap() const { return m_has_smap; }
  bool has_xsave() const { return m_has_xsave; }
  bool has_avx() const { return m_has_avx; }
  bool has_avx2() const { return m_has_avx2; }
  bool has_avx512() const { return m_has_avx512; }
  bool has_1gb_pages() const { return m_has_1gb_pages; }
  bool has_invpcid() const { return m_has_invpcid; }
  bool has_fsgsbase() const { return m_has_fsgsbase; }
  bool has_umip() const { return m_has_umip; }
  bool has_la57() const { return m_has_la57; }
  bool has_cet() const { return m_has_cet; }
  uint8_t phys_addr_width() const { return m_phys_addr_width; }
  uint8_t virt_addr_width() const { return m_virt_addr_width; }
  uint8_t lapic_id() const { return m_lapic_id; }

  void initialize_features();

  uint64_t read_msr(uint32_t msr);
  void write_msr(uint32_t msr, uint64_t value);
  uint64_t read_register(CpuRegister reg);
  void write_register(CpuRegister reg, uint64_t value);
};
