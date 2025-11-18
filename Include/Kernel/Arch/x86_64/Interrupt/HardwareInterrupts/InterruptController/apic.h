#pragma once

#include "LibFK/Container/string.h"
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <LibFK/Types/types.h>

/**
 * @brief Local APIC controller for x86_64
 *
 * Implements HardwareInterrupt interface for Strategy pattern
 */
class APIC : public HardwareInterrupt {
private:
  uintptr_t lapic_base = 0;       ///< Mapped LAPIC base
  uint64_t apic_ticks_per_ms = 0; ///< Timer ticks per ms

  uint32_t read(uint32_t reg);
  void write(uint32_t reg, uint32_t value);

  fk::text::String m_name = "APIC";

public:
  static APIC &the() {
    static APIC inst;
    return inst;
  }

  uint64_t get_ticks_per_ms() const { return apic_ticks_per_ms; }

  fk::text::String get_name() override { return m_name; }
  /**
   * @brief Initialize and enable the local APIC
   */
  void initialize() override;

  /**
   * @brief Send an End-of-Interrupt (EOI)
   */
  void send_eoi(uint8_t irq = 0) override;

  /**
   * @brief Mask a specific interrupt (not typical for LAPIC)
   */
  void mask_interrupt(uint8_t irq) override;

  /**
   * @brief Unmask a specific interrupt (not typical for LAPIC)
   */
  void unmask_interrupt(uint8_t irq) override;

  /**
   * @brief Calibrate APIC timer
   */
  void calibrate_timer();

  /**
   * @brief Configure periodic APIC timer
   */
  void setup_timer(uint64_t ms);
};
