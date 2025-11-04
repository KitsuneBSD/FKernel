#pragma once

#include <LibFK/Types/types.h>

// I/O APIC Registers
constexpr uint32_t IOAPIC_REG_ID = 0x00;
constexpr uint32_t IOAPIC_REG_VER = 0x01;
constexpr uint32_t IOAPIC_REG_ARB = 0x02;
constexpr uint32_t IOAPIC_REG_TABLE_BASE = 0x10;
constexpr uintptr_t IOAPIC_ADDRESS = 0xFEC00000;

/**
 * @brief Represents an I/O APIC controller.
 *
 * This class manages a single I/O APIC, allowing for redirection
 * of hardware interrupts to specific processor cores.
 */
class IOAPIC {
private:
  uintptr_t ioapic_base = 0; ///< Base address of mapped I/O APIC registers
  uint32_t global_interrupt_base =
      0; ///< Global System Interrupt base for this IOAPIC

  /**
   * @brief Reads a value from an I/O APIC register.
   * @param reg The register offset.
   * @return The value of the register.
   */
  uint32_t read(uint32_t reg);

  /**
   * @brief Writes a value to an I/O APIC register.
   * @param reg The register offset.
   * @param value The value to write.
   */
  void write(uint32_t reg, uint32_t value);

public:
  IOAPIC() = default;

  static IOAPIC &the() {
    static IOAPIC inst;
    return inst;
  }

  /**
   * @brief Initialize the I/O APIC
   * @param base_addr Physical base address of the I/O APIC
   * @param gsi_base Global System Interrupt base
   */
  void initialize(uintptr_t base_addr);

  /**
   * @brief Remaps an IRQ to a specific vector and CPU.
   * @param irq The IRQ number to remap.
   * @param vector The interrupt vector to trigger.
   * @param lapic_id The local APIC ID of the destination CPU.
   * @param flags Interrupt flags (trigger mode, polarity, etc.).
   */
  void remap_irq(uint8_t irq, uint8_t vector, uint8_t lapic_id, uint32_t flags);

  /**
   * @brief Masks (disables) a specific IRQ line.
   * @param irq IRQ number (relative to GSI base).
   */
  void mask_irq(uint8_t irq);

  /**
   * @brief Unmasks (enables) a specific IRQ line.
   * @param irq IRQ number (relative to GSI base).
   */
  void unmask_irq(uint8_t irq);
};
