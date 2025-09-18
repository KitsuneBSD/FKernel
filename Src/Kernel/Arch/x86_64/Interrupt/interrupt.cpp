#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupt/pic.h>
#include <Kernel/Arch/x86_64/Interrupt/handlerList.h>
#include <Kernel/Arch/x86_64/Interrupt/idt.h>
#include <Kernel/Arch/x86_64/Interrupt/idt_types.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>

void init_idt() {
  g_idt.clear();

  // NOTE: Start loading exceptions between isr0..isr32
  for (size_t i = 0; i < MAX_X86_64_ISR_SIZE; ++i) {
    g_idt.set_gate(i, g_isr_stubs[i]);
    g_idt.register_irq_handler(i, default_handler);
  }

  // NOTE: Start loading routines between irq0..irq15
  for (size_t j = 0; j < MAX_X86_64_IRQ_SIZE; ++j) {
    size_t irq = 32 + j;
    g_idt.set_gate(irq, g_irq_stubs[irq]);
    g_idt.register_irq_handler(irq, default_handler);
  }

  g_idt.load();

  Pic8259::remap(0x20, 0x28);
  Pic8259::set_mask_master(0xFC);
  Pic8259::set_mask_slave(0xFF);

  kprintf("IDT Loaded\n");
}
