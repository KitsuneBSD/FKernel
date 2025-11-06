#pragma once

#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

// TODO: USE Strategy to select between 8259Pic and APIC

/**
 * @brief Master PIC command port
 */
static constexpr uint8_t PIC1_CMD = 0x20;

/**
 * @brief Master PIC data port
 */
static constexpr uint8_t PIC1_DATA = 0x21;

/**
 * @brief Slave PIC command port
 */
static constexpr uint8_t PIC2_CMD = 0xA0;

/**
 * @brief Slave PIC data port
 */
static constexpr uint8_t PIC2_DATA = 0xA1;

/**
 * @brief Initialization Control Word 1 flags
 */
static constexpr uint8_t ICW1_INIT = 0x11;
static constexpr uint8_t ICW1_ICW4 = 0x01;

/**
 * @brief Initialization Control Word 4 flags
 */
static constexpr uint8_t ICW4_8086 = 0x01;

/**
 * @brief Read IRR (Interrupt Request Register) command
 */
static constexpr uint8_t PIC_READ_IRR = 0x0A;

/**
 * @brief Read ISR (In-Service Register) command
 */
static constexpr uint8_t PIC_READ_ISR = 0x0B;

/**
 * @brief Reads either the IRR or ISR from both master and slave PICs.
 *
 * @param ocw3 Command to select IRR or ISR
 * @return Combined 16-bit register value (high byte = slave, low byte = master)
 */
static uint16_t __get_irq_reg(uint8_t ocw3) {
  outb(PIC1_CMD, ocw3);
  outb(PIC2_CMD, ocw3);
  return (inb(PIC2_CMD) << 8) | inb(PIC1_CMD);
}

/**
 * @brief Represents the Intel 8259 Programmable Interrupt Controller (PIC)
 *
 * This class provides functions to initialize the PIC, mask/unmask IRQs,
 * and send End-of-Interrupt (EOI) signals.
 */
class PIC8259 {
private:
  /**
   * @brief Get the current Interrupt Request Register (IRR)
   *
   * @return 16-bit IRR (master = low byte, slave = high byte)
   */
  static uint16_t get_irr() { return __get_irq_reg(PIC_READ_IRR); }

  /**
   * @brief Get the current In-Service Register (ISR)
   *
   * @return 16-bit ISR (master = low byte, slave = high byte)
   */
  static uint16_t get_isr() { return __get_irq_reg(PIC_READ_ISR); }

public:
  /**
   * @brief Initialize the PICs and remap IRQs to avoid conflicts with CPU
   * exceptions
   */
  static void initialize();

  /**
   * @brief Mask (disable) a specific IRQ line
   *
   * @param irq IRQ number (0-15)
   */
  static void mask_irq(uint8_t irq);

  /**
   * @brief Unmask (enable) a specific IRQ line
   *
   * @param irq IRQ number (0-15)
   */
  static void unmask_irq(uint8_t irq);

  /**
   * @brief Send an End-of-Interrupt (EOI) signal to the PICs
   *
   * @param irq IRQ number that has been handled
   */
  static void send_eoi(uint8_t irq);

  /**
   * @brief Safely send an EOI if the IRQ is currently in service
   *
   * @param irq IRQ number to send EOI for
   */
  static void send_eoi_safe(uint8_t irq);

  /**
   * @brief Disable the PIC by masking all IRQ lines.
   */
  static void disable() {
    outb(PIC1_DATA, 0xFF); // Mask all IRQs on master PIC
    outb(PIC2_DATA, 0xFF); // Mask all IRQs on slave PIC
    klog("PIC8259", "PIC disabled by masking all IRQs.");
  }
};
