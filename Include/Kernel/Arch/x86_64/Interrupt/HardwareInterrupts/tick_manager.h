#pragma once

#include <LibFK/Types/types.h>

class TickManager {
private:
  uint64_t m_ticks = 0;
  uint32_t m_frequency = 0;

public:
  TickManager() = default;

  static TickManager &the() {
    static TickManager inst;
    return inst;
  }

  void initialize();

  /**
   * @brief Halts the CPU for a specified duration in milliseconds.
   *
   * This function puts the CPU into a halted state (HLT) until timer
   * interrupts advance the internal tick counter. It enables interrupts
   * temporarily (if they were disabled) so that HLT will wake on the next
   * interrupt, and restores the original interrupt state on return.
   *
   * The duration is specified in milliseconds and is converted to tick counts
   * using the configured `m_frequency` (Hz). If `m_frequency` is 0 a simple
   * busy-wait is used.
   *
   * @param ms Number of milliseconds to sleep.
   */
  void sleep(uint64_t ms);
  void increment_ticks() { m_ticks++; };
  uint64_t get_ticks() { return m_ticks; }
  void set_frequency(uint32_t frequency) { m_frequency = frequency; }
};
