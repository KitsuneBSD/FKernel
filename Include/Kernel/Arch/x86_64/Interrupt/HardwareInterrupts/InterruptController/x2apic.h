#pragma once

#include <LibFK/Text/string.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <LibFK/Types/types.h>

/**
 * @brief x2APIC controller for x86_64
 *
 * Implements HardwareInterrupt interface for Strategy pattern
 */
class X2APIC : public HardwareInterrupt {
private:
  uint64_t apic_ticks_per_ms = 0; ///< Timer ticks per ms
  fk::text::String m_name = "x2APIC";
  bool m_is_initialized = false;

public:
  static X2APIC &the() {
    static X2APIC inst;
    return inst;
  }

  uint64_t get_ticks_per_ms() const { return apic_ticks_per_ms; }

  uint32_t get_id() const;

  void send_ipi(uint8_t lapic_id, uint8_t vector, uint32_t delivery_mode);
  void wait_ipi_delivery();

  fk::text::String get_name() override { return m_name; }
  /**
   * @brief Initialize and enable the local x2APIC (BSP only)
   */
  void initialize() override;

  /**
   * @brief Enable x2APIC mode on an AP (no singleton guard).
   * Per SDM Vol.3A §10.12.5.1: set IA32_APIC_BASE[10:11] then enable SVR.
   */
  void initialize_on_ap();

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

  fk::core::Result<uint8_t, fk::core::Error> allocate_msi_vector(const PciDevice& device) override;
  void enable_msi(uint8_t vector) override;
  void disable_msi(uint8_t vector) override;

  /**
   * @brief Calibrate APIC timer
   */
  void calibrate_timer();

  /**
   * @brief Configure periodic APIC timer at the given frequency.
   * @param frequency_hz Interrupts per second (e.g., 1000 = 1 kHz tick)
   */
  void setup_timer(uint64_t frequency_hz);
};
