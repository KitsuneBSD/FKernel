#include <Kernel/Arch/x86_64/Interrupt/handlerList.h>
#include <Kernel/Arch/x86_64/Interrupt/idt.h>
#include <Kernel/Arch/x86_64/Interrupt/idt_types.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>

void init_idt() {
  g_idt.clear();

  // NOTE: Start loading exceptions between isr0..isr32
  for (uint16_t n = 0; n < MAX_X86_64_ISR_SIZE; ++n) {
    g_idt.set_gate(n, g_isr_stubs[n]);
    g_idt.register_isr_handler(n, default_handler);
  }

  g_idt.load();

  kprintf("IDT Loaded\n");
}
