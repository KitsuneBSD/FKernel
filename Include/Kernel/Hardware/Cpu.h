#pragma once

#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

/**
 * @brief CPU feature detection and MSR access
 *
 * Provides a singleton interface for querying CPU features (like APIC and
 * hypervisor support) and reading/writing Model-Specific Registers (MSRs).
 */
class CPU {
private:
  bool h_apic = false;       ///< True if CPU has local APIC
  bool h_hypervisor = false; ///< True if running under a hypervisor

  /**
   * @brief Private constructor for singleton
   *
   * Performs CPUID detection to set APIC and hypervisor flags.
   */
  CPU() {
    uint64_t a, b, c, d;
    asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(1));
    h_apic = d & (1 << 9);
    h_hypervisor = c & (1 << 31);
  }

public:
  /**
   * @brief Get the singleton CPU instance
   * @return Reference to CPU instance
   */
  static CPU &the() {
    static CPU inst;
    return inst;
  }

  /**
   * @brief Write to a Model-Specific Register (MSR)
   * @param msr MSR address
   * @param value Value to write
   */
  void write_msr(uint32_t msr, uint64_t value);

  /**
   * @brief Read a Model-Specific Register (MSR)
   * @param msr MSR address
   * @return Value read from the MSR
   */
  uint64_t read_msr(uint32_t msr);

  /**
   * @brief Check if the CPU has a local APIC
   * @return true if APIC is available
   */
  bool has_apic() { return this->h_apic; }

  /**
   * @brief Check if the CPU is running under a hypervisor
   * @return true if hypervisor is present
   */
  bool has_hypervisor() { return this->h_hypervisor; }
};
