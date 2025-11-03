#pragma once

#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

/**
 * @brief Model-specific register (MSR) for the APIC base address
 */
constexpr uint32_t APIC_BASE_MSR = 0x1B;

/**
 * @brief Bit to enable the APIC
 */
constexpr uint32_t APIC_ENABLE = 1 << 11;

/**
 * @brief Bit indicating the BSP (Bootstrap Processor)
 */
constexpr uint32_t APIC_BSP = 1 << 8;

/**
 * @brief APIC Spurious Interrupt Vector Register offset
 */
constexpr uint32_t APIC_SPURIOUS = 0xF0;

/**
 * @brief Spurious Vector Register Enable flag (bit 8)
 */
constexpr uint32_t APIC_SVR_ENABLE = 0x100;

/**
 * @brief Memory-mapped APIC register range size
 */
constexpr uintptr_t APIC_RANGE_SIZE = 0x1000;

/**
 * @brief Local APIC Timer LVT Mode: Periodic
 */
constexpr uint32_t APIC_LVT_TIMER_MODE_PERIODIC = (1 << 17);

/**
 * @brief Local APIC Timer Divisor
 */
constexpr uint32_t APIC_TIMER_DIVISOR = 0x3;

/**
 * @brief Represents the Local APIC memory-mapped registers.
 *
 * This struct is mapped directly to the APIC's MMIO range and
 * allows reading and writing to control the local APIC.
 */
struct local_apic {
  volatile uint32_t id;            ///< Local APIC ID register
  volatile uint32_t version;       ///< Local APIC Version register
  volatile uint32_t tpr;           ///< Task Priority Register
  volatile uint32_t apr;           ///< Arbitration Priority Register
  volatile uint32_t ppr;           ///< Processor Priority Register
  volatile uint32_t eoi;           ///< End-of-Interrupt register
  volatile uint32_t ldr;           ///< Logical Destination Register
  volatile uint32_t dfr;           ///< Destination Format Register
  volatile uint32_t spurious;      ///< Spurious Interrupt Vector Register
  volatile uint32_t isr[8];        ///< In-Service Register (256 bits)
  volatile uint32_t tmr[8];        ///< Trigger Mode Register
  volatile uint32_t irr[8];        ///< Interrupt Request Register (256 bits)
  volatile uint32_t esr;           ///< Error Status Register
  volatile uint32_t icr_low;       ///< Interrupt Command Register (low)
  volatile uint32_t icr_high;      ///< Interrupt Command Register (high)
  volatile uint32_t lvt_timer;     ///< Local Vector Table: Timer
  volatile uint32_t lvt_thermal;   ///< Local Vector Table: Thermal sensor
  volatile uint32_t lvt_perf;      ///< Local Vector Table: Performance counter
  volatile uint32_t lvt_lint0;     ///< Local Vector Table: LINT0
  volatile uint32_t lvt_lint1;     ///< Local Vector Table: LINT1
  volatile uint32_t lvt_error;     ///< Local Vector Table: Error
  volatile uint32_t initial_count; ///< Timer Initial Count register
  volatile uint32_t current_count; ///< Timer Current Count register
  volatile uint32_t divide_config; ///< Timer Divide Configuration register
};

/**
 * @brief Represents the Local APIC controller.
 *
 * This singleton class manages the local APIC, including enabling it,
 * sending EOIs, and configuring/calibrating the APIC timer.
 */
class APIC {
private:
  APIC() = default;            ///< Private constructor for singleton
  local_apic *lapic = nullptr; ///< Pointer to the mapped local APIC registers
  uint64_t apic_ticks_per_ms = 0; ///< APIC timer ticks per millisecond

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
