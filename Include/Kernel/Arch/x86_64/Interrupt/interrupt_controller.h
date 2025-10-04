#pragma once

#include <Kernel/Arch/x86_64/Interrupt/interrupt_types.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/array.h>

class InterruptController {
private:
  array<idt_entry, MAX_x86_64_IDT_SIZE> m_entries;
  array<interrupt, MAX_x86_64_IDT_SIZE> m_handlers;

  InterruptController() { clear(); }

  bool is_interrupt_enable = true;

  void enable_interrupt() {
    if (is_interrupt_enable) {
      klog("INTERRUPT", "Interrupts already enabled");
      return;
    }

    asm volatile("sti");
    is_interrupt_enable = true;
    klog("INTERRUPT", "Interrupts enabled");
  }

  void disable_interrupt() {
    if (!is_interrupt_enable) {
      klog("INTERRUPT", "Interrupts already disabled");
      return;
    }

    asm volatile("cli");
    is_interrupt_enable = false;
    klog("INTERRUPT", "Interrupts disabled");
  }

public:
  static InterruptController &the() {
    static InterruptController instance;
    return instance;
  }

  void initialize();

  void set_gate(uint8_t vector, void (*new_interrupt)(),
                uint16_t selector = 0x08, uint8_t ist = 0,
                uint8_t type_attr = 0x8E);

  void clear();
  void load();

  void register_interrupt(interrupt new_interrupt, uint8_t vector);
  interrupt get_interrupt(uint8_t vector);

  idt_entry *raw_entries() { return m_entries.begin(); }

  const idt_entry *raw_entries() const { return m_entries.begin(); };
};
