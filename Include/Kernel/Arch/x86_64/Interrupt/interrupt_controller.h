#pragma once

#include <Kernel/Arch/x86_64/Interrupt/interrupt_types.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/array.h>

/**
 * @brief x86_64 Interrupt Controller
 *
 * This singleton class manages the Interrupt Descriptor Table (IDT),
 * including setting gates, registering handlers, and enabling/disabling
 * CPU interrupts.
 */
class InterruptController {
private:
  /// Array of IDT entries
  array<idt_entry, MAX_x86_64_IDT_SIZE> m_entries;

  /// Array of registered interrupt handlers
  array<interrupt, MAX_x86_64_IDT_SIZE> m_handlers;

  /// Private constructor for singleton; clears all entries
  InterruptController() { clear(); }

  /// Tracks whether interrupts are enabled
  bool is_interrupt_enable = true;

  /**
   * @brief Enable CPU interrupts
   *
   * Uses the `sti` instruction. Logs a warning if interrupts are already
   * enabled.
   */
  void enable_interrupt() {
    if (is_interrupt_enable) {
      kwarn("INTERRUPT", "Interrupts already enabled");
      return;
    }

    asm volatile("sti");
    is_interrupt_enable = true;
    kdebug("INTERRUPT", "Interrupts enabled");
  }

  /**
   * @brief Disable CPU interrupts
   *
   * Uses the `cli` instruction. Logs a warning if interrupts are already
   * disabled.
   */
  void disable_interrupt() {
    if (!is_interrupt_enable) {
      kwarn("INTERRUPT", "Interrupts already disabled");
      return;
    }

    asm volatile("cli");
    is_interrupt_enable = false;
    kdebug("INTERRUPT", "Interrupts disabled");
  }

public:
  /**
   * @brief Get the singleton instance of the InterruptController
   *
   * @return Reference to the InterruptController instance
   */
  static InterruptController &the() {
    static InterruptController instance;
    return instance;
  }

  /**
   * @brief Initialize the IDT
   *
   * Sets all entries to a default state and prepares the controller for use.
   */
  void initialize();

  /**
   * @brief Set an IDT gate
   *
   * @param vector Interrupt vector number
   * @param new_interrupt Pointer to the new interrupt handler function
   * @param selector Code segment selector (default 0x08)
   * @param ist Interrupt Stack Table index (default 0)
   * @param type_attr Type and attributes of the IDT gate (default 0x8E)
   */
  void set_gate(uint8_t vector, void (*new_interrupt)(),
                uint16_t selector = 0x08, uint8_t ist = 0,
                uint8_t type_attr = 0x8E);

  /**
   * @brief Clear all IDT entries and handlers
   */
  void clear();

  /**
   * @brief Load the IDT into the CPU
   *
   * Executes the `lidt` instruction with the current entries.
   */
  void load();

  /**
   * @brief Register a high-level interrupt handler
   *
   * @param new_interrupt Interrupt handler function
   * @param vector Interrupt vector number
   */
  void register_interrupt(interrupt new_interrupt, uint8_t vector);

  /**
   * @brief Get the registered interrupt handler for a vector
   *
   * @param vector Interrupt vector number
   * @return Registered interrupt handler
   */
  interrupt get_interrupt(uint8_t vector);

  /**
   * @brief Get a pointer to the raw IDT entries (mutable)
   *
   * @return Pointer to IDT entries
   */
  idt_entry *raw_entries() { return m_entries.begin(); }

  /**
   * @brief Get a pointer to the raw IDT entries (const)
   *
   * @return Pointer to IDT entries
   */
  const idt_entry *raw_entries() const { return m_entries.begin(); };
};
