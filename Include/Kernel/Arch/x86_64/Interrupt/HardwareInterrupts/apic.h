#pragma once

#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

// Model-specific register (MSR) for the APIC base address
constexpr uint32_t APIC_BASE_MSR = 0x1B;

// Bit to enable the APIC in the MSR
constexpr uint32_t APIC_MSR_ENABLE = 1 << 11;

// Bit indicating the BSP (Bootstrap Processor) in the MSR
constexpr uint32_t APIC_MSR_BSP = 1 << 8;

// Spurious Vector Register Enable flag (bit 8)
constexpr uint32_t APIC_SVR_ENABLE = 1 << 8;

// Memory-mapped APIC register range size
constexpr uintptr_t APIC_RANGE_SIZE = 0x1000;

// Local APIC Timer LVT Mode: Periodic
constexpr uint32_t APIC_LVT_TIMER_MODE_PERIODIC = (1 << 17);

// Local APIC Timer Divisor (divide by 16)
constexpr uint32_t APIC_TIMER_DIVISOR = 0x3;

constexpr uint32_t ID = 0x20;
constexpr uint32_t VERSION = 0x30;
constexpr uint32_t TPR = 0x80;
constexpr uint32_t APR = 0x90;
constexpr uint32_t PPR = 0xA0;
constexpr uint32_t EOI = 0xB0;
constexpr uint32_t RRD = 0xC0;
constexpr uint32_t LDR = 0xD0;
constexpr uint32_t DFR = 0xE0;
constexpr uint32_t SPURIOUS = 0xF0;
constexpr uint32_t ISR_START = 0x100;
constexpr uint32_t TMR_START = 0x180;
constexpr uint32_t IRR_START = 0x200;
constexpr uint32_t ESR = 0x280;
constexpr uint32_t ICR_LOW = 0x300;
constexpr uint32_t ICR_HIGH = 0x310;
constexpr uint32_t LVT_TIMER = 0x320;
constexpr uint32_t LVT_THERMAL = 0x330;
constexpr uint32_t LVT_PERF = 0x340;
constexpr uint32_t LVT_LINT0 = 0x350;
constexpr uint32_t LVT_LINT1 = 0x360;
constexpr uint32_t LVT_ERROR = 0x370;
constexpr uint32_t INITIAL_COUNT = 0x380;
constexpr uint32_t CURRENT_COUNT = 0x390;
constexpr uint32_t DIVIDE_CONFIG = 0x3E0;

/**
 * @brief Represents the Local APIC controller.
 *
 * This singleton class manages the local APIC, including enabling it,
 * sending EOIs, and configuring/calibrating the APIC timer.
 */
class APIC {
private:
  APIC() = default;         ///< Private constructor for singleton
  uintptr_t lapic_base = 0; ///< Base address of mapped local APIC registers
  uint64_t apic_ticks_per_ms = 0; ///< APIC timer ticks per millisecond

  /**
   * @brief Reads a value from a local APIC register.
   * @param reg The register offset.
   * @return The value of the register.
   */
  uint32_t read(uint32_t reg);

  /**
   * @brief Writes a value to a local APIC register.
   * @param reg The register offset.
   * @param value The value to write.
   */
  void write(uint32_t reg, uint32_t value);

public:
  /**
   * @brief Get the singleton instance of the APIC
   * @return Reference to the APIC instance
   */
  static APIC &the() {
    static APIC inst;
    return inst;
  }

  /**
   * @brief Enable the local APIC
   *
   * Maps the APIC registers and sets the enable bit.
   */
  void enable();

  /**
   * @brief Send an End-of-Interrupt (EOI) to the local APIC
   */
  void send_eoi();

  /**
   * @brief Calibrate the APIC timer against the PIT or other time source
   */
  void calibrate_timer();

  /**
   * @brief Configure the APIC timer for a periodic interval
   *
   * @param ms Interval in milliseconds
   */
  void setup_timer(uint64_t ms);

  /**
   * @brief Get the number of APIC timer ticks per millisecond
   *
   * @return Ticks per millisecond
   */
  uint64_t get_ticks_per_ms() const { return apic_ticks_per_ms; }
};
