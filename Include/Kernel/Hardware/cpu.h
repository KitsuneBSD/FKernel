#pragma once

#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/string.h>
#include <LibFK/Types/types.h>

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

  uint64_t read_msr(uint32_t msr);
  void write_msr(uint32_t msr, uint64_t value);
};
