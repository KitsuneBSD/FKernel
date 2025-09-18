#pragma once

#include <Kernel/Arch/x86_64/Interrupt/idt_types.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>
#include <Kernel/Arch/x86_64/asm.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/stdio.h>
#include <LibFK/array.h>

class idt {
private:
  array<Idt_Entry, MAX_X86_64_IDT_SIZE> entries;
  isr_handler_t isr_handlers[MAX_X86_64_ISR_SIZE];
  irq_handler_t irq_handlers[MAX_X86_64_IRQ_SIZE];

public:
  idt();

  void clear();
  void set_gate(uint8_t vector, void (*handler_ptr)(), uint16_t selector = 0x08,
                uint8_t ist = 0, uint8_t type_attr = 0x8E);

  void load();

  void register_isr_handler(uint8_t vector, isr_handler_t handler);
  isr_handler_t get_isr_handler(uint8_t vec) const;

  void register_irq_handler(uint8_t vector, irq_handler_t handler);
  irq_handler_t get_irq_handler(uint8_t vec) const;

  Idt_Entry *raw_entries();
  const Idt_Entry *raw_entries() const;
};

static idt g_idt;

void init_idt();
