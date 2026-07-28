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
  uint8_t lapic_id() const { return m_lapic_id; }

  void initialize_features();

  uint64_t read_msr(uint32_t msr);
  void write_msr(uint32_t msr, uint64_t value);
  uint64_t read_register(CpuRegister reg);
  void write_register(CpuRegister reg, uint64_t value);
};
